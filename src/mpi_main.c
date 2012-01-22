//---------------------------------------------------------------------------
// mpi_main.c
//
// Main procedures for initiating the parallel version of SUBDUE using MPI.
//
// SUBDUE 5
//---------------------------------------------------------------------------

#include <time.h>
#include <mpi.h>
#include "subdue.h"

// Function prototypes

int main(int, char **);
void SubdueMaster(Parameters *);
void SubdueChild(Parameters *);
Parameters *GetParameters(int, char **, int);
void PrintParameters(Parameters *);
void FreeParameters(Parameters *);


//---------------------------------------------------------------------------
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main parallel MPI SUBDUE function that processes command-line
// arguments and initiates discovery.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   time_t startTime;
   time_t endTime;
   Parameters *parameters;
   int processRank;
   int numProcesses;

   // Start up MPI; determine process rank and number of processes
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &processRank);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);

   if (processRank == 0) 
   { // master process
      startTime = time(NULL);
      printf("MPI SUBDUE %s\n\n", SUBDUE_VERSION);
      parameters = GetParameters(argc, argv, processRank);
      parameters->numPartitions = numProcesses - 1;
      PrintParameters(parameters);
      SubdueMaster(parameters);
      FreeParameters(parameters);
      endTime = time(NULL);
      printf("SUBDUE done (elapsed time = %lu seconds).\n",
             (endTime - startTime));
   } 
   else 
   { // child process
      parameters = GetParameters(argc, argv, processRank);
      parameters->numPartitions = numProcesses - 1;
      SubdueChild(parameters);
      FreeParameters(parameters);
   }

   // shut down MPI
   MPI_Finalize();

   return 0;
}


//---------------------------------------------------------------------------
// NAME: SubdueMaster
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Controls the master MPI process, which receives substructures
// from the child processes, sends them for evaluation to other child
// processes, identifies the best substructures, and informs each child of
// the best substructure for compression and further discover if multiple
// iterations.
//---------------------------------------------------------------------------

void SubdueMaster(Parameters *parameters)
{
   time_t iterationStartTime;
   time_t iterationEndTime;
   FILE *outputFile;
   Substructure *childSub;
   Substructure **childSubs;
   SubList *subList;
   double subValue;
   ULONG i, j;
   ULONG iteration;
   ULONG numInstances;
   ULONG numNegInstances;
   BOOLEAN done;
   int childID;

   // parameters used
   ULONG numPartitions = parameters->numPartitions;
   ULONG iterations = parameters->iterations;
   LabelList *labelList = parameters->labelList;
   ULONG numBestSubs = parameters->numBestSubs;

   childSubs = (Substructure **) malloc
               ((numPartitions + 1) * sizeof(Substructure *));
   if (childSubs == NULL)
      OutOfMemoryError("childSubs");

   if (iterations > 1)
      printf("----- Iteration 1 -----\n\n");

   iteration = 1;
   done = FALSE;
   while ((iteration <= iterations) && (! done)) 
   {
      iterationStartTime = time(NULL);
      if (iteration > 1)
         printf("----- Iteration %lu -----\n\n", iteration);
      for (i = 1; i <= numPartitions; i++)
         childSubs[i] = NULL;
      subList = AllocateSubList();

      // receive substructures from child processes
      for (i = 1; i <= numPartitions; i++) 
      {
         childSub = ReceiveBestSubFromChild(&childID, parameters);
         printf("Received substructure from child %d:\n", childID);
         PrintSub(childSub, parameters);
         printf("\n");
         if (childSub != NULL) 
         {
            if (UniqueChildSub(childSub, childSubs, parameters))
               childSubs[childID] = childSub;
            else 
               FreeSub(childSub);
         }
      }
      // send unique substructures to other children for evaluation
      for (i = 1; i <= numPartitions; i++) 
      {
         for (j = 1; j <= numPartitions; j++) 
         {
            if (i != j)
               SendEvalSubToChild(childSubs[i], parameters, j);
               printf ("Sent substructure from child %ld to child %ld\n", i, j);
         }
         for (j = 1; j < numPartitions; j++) 
         {
            // Here we simply add the values from each partition to arrive at
            // the global value of a substructure.  Another approach would be
            // to compute a value for the whole graph based on the sizes of
            // the individual whole and compressed partition graphs, but this
            // would require more work and the outcome will most likely be
            // the same. (*****)
            ReceiveEvalFromChild(&subValue, &numInstances, &numNegInstances);
            printf ("Received child %ld substructure's value from some child (count=%ld)\n", i, j);
            if (childSubs[i] != NULL) 
            {
               childSubs[i]->value += subValue;
               childSubs[i]->numInstances += numInstances;
               childSubs[i]->numNegInstances += numNegInstances;
            }
         }
         if (childSubs[i] != NULL)
            SubListInsert(childSubs[i], subList, numBestSubs, FALSE, labelList);
      }

      if (subList->head == NULL) 
      {
         done = TRUE;
         printf("No substructures found.\n\n");
         SendBestSubToChildren(NULL, parameters); // stops child processes
      } 
      else 
      {
         // write output to stdout
         if (parameters->outputLevel > 1) 
         {
            printf("\nBest substructures (max %lu):\n\n", numBestSubs);
            PrintSubList(subList, parameters);
         } 
         else 
         {
            printf("\nBest substructure:\n\n");
            PrintSub(subList->head->sub, parameters);
         }

         // write machine-readable output to file, if given
         if (parameters->outputToFile) 
         {
            outputFile = fopen(parameters->outFileName, "a");
            if (outputFile == NULL) 
            {
               printf("WARNING: unable to write to output file %s, disabling\n",
                      parameters->outFileName);
               parameters->outputToFile = FALSE;
            }
            WriteGraphToFile(outputFile, subList->head->sub->definition,
                             labelList, 0, 0, 
                             subList->head->sub->definition->numVertices, TRUE);
   
            fclose(outputFile);
         }

         if (iteration < iterations) 
         { // another iteration?
           // compress child graphs with best substructure and restart
            SendBestSubToChildren(subList->head->sub, parameters);
         }
      }
      FreeSubList(subList);

      if (iterations > 1) 
      {
         iterationEndTime = time(NULL);
         printf("Elapsed time for iteration %lu = %lu seconds.\n\n",
                iteration, (iterationEndTime - iterationStartTime));
      }
      iteration++;
   }
}


//---------------------------------------------------------------------------
// NAME: SubdueChild
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Controls the child MPI process, which discovers the best
// substructure in the child's partition of the graph, sends the
// substructure to the master, evaluates substructures from other
// partitions, sends their values to the master, receives the globally best
// substructure, and compresses (or removes) the positive graphs.  If
// multiple iterations are requested, then process is repeated.  If the
// master ever sends a NULL best substructure (or iterations complete),
// then that signals termination of the process.
//---------------------------------------------------------------------------

void SubdueChild(Parameters *parameters)
{
   SubList *subList;
   SubList *evalSubs;
   Substructure *evalSub;
   Substructure *masterSub;
   Substructure *bestSub;
   ULONG i;
   ULONG iteration;
   BOOLEAN done;
   BOOLEAN stopCondition;

   // parameters used
   Graph *posGraph = parameters->posGraph;
   Graph *negGraph = parameters->negGraph;
   ULONG numPartitions = parameters->numPartitions;
   ULONG iterations = parameters->iterations;
   ULONG numPreSubs = parameters->numPreSubs;
   LabelList *labelList = parameters->labelList;
   ULONG evalMethod = parameters->evalMethod;

   // compress pos and neg graphs with predefined subs, if given
   if (numPreSubs > 0)
      CompressWithPredefinedSubs(parameters);

   iteration = 1;
   done = FALSE;
   stopCondition = FALSE;
   while ((iteration <= iterations) && (! done)) 
   {
      evalSubs = AllocateSubList();
      if (stopCondition)
         subList = AllocateSubList();
      else 
         subList = DiscoverSubs(parameters);
      // send best substructure to master and retain
      if (subList->head == NULL) 
      {
         SendBestSubToMaster(NULL, parameters); // no substructures found
      } 
      else 
      {
         SendBestSubToMaster(subList->head->sub, parameters);
         SubListInsert(subList->head->sub, evalSubs, 0, FALSE, labelList);
         subList->head->sub = NULL;
      }
      FreeSubList(subList);
      // receive substructures (possibly NULL) from all other partitions,
      // evaluate them, send value back to master, and retain for later
      for (i = 1; i < numPartitions; i++) 
      {
         evalSub = ReceiveEvalSubFromMaster(parameters);
         if (evalSub == NULL) 
         {
            SendEvalToMaster(0.0, 0, 0);
         } 
         else 
         {
            evalSub->instances =
               FindInstances(evalSub->definition, posGraph, parameters);
            evalSub->numInstances = CountInstances(evalSub->instances);
            if (negGraph != NULL) 
            {
               evalSub->negInstances =
                  FindInstances(evalSub->definition, negGraph, parameters);
               evalSub->numNegInstances = CountInstances(evalSub->negInstances);
            }
            EvaluateSub(evalSub, parameters);
            SendEvalToMaster(evalSub->value, evalSub->numInstances,
                             evalSub->numNegInstances);
            SubListInsert(evalSub, evalSubs, 0, FALSE, labelList);
            // ***** Retract new labels (but may clobber label indices in a
            // ***** retained evalSub).
         }
      }

      if (iteration < iterations) 
      { // another iteration?
         masterSub = ReceiveBestSubFromMaster(parameters);
         if (masterSub == NULL) 
         {
            done = TRUE; // signal to end child process
         } 
         else 
         {
            // compress partition graph(s)
            bestSub = FindEvalSub(masterSub, evalSubs, parameters);
            if (evalMethod == EVAL_SETCOVER)
               RemovePosEgsCovered(bestSub, parameters);
            else
               CompressFinalGraphs(bestSub, parameters, iteration, FALSE);
            FreeSub(masterSub);
         }
         // check for stopping condition (i.e., no reason to call discoverSubs)
         if (evalMethod == EVAL_SETCOVER) 
         {
            if (parameters->numPosEgs == 0) 
            {
              stopCondition = TRUE; // preempt discovery, all pos egs covered
            }
         } 
         else 
         {
            if (parameters->posGraph->numEdges == 0) 
            {
               stopCondition = TRUE; //preempt discovery, graph fully compressed
            }
         }
      }
      FreeSubList(evalSubs);
      iteration++;
   }
}


//---------------------------------------------------------------------------
// NAME: GetParameters
//
// INPUTS: (int argc) - number of command-line arguments
//         (char *argv[]) - array of command-line argument strings
//         (int processRank) - rank of process (master = 0)
//
// RETURN: (Parameters *)
//
// PURPOSE: Initialize parameters structure and process command-line
// options.
//---------------------------------------------------------------------------

Parameters *GetParameters(int argc, char *argv[], int processRank)
{
   Parameters *parameters;
   int i;
   double doubleArg;
   ULONG ulongArg;
   BOOLEAN limitSet = FALSE;
   FILE *outputFile;

   parameters = (Parameters *) malloc(sizeof(Parameters));
   if (parameters == NULL)
      OutOfMemoryError("parameters");

   // initialize default parameter settings
   parameters->directed = TRUE;
   parameters->limit = 0;
   parameters->numBestSubs = 3;
   parameters->beamWidth = 4;
   parameters->valueBased = FALSE;
   parameters->prune = FALSE;
   strcpy(parameters->outFileName, "none");
   parameters->outputToFile = FALSE;
   parameters->outputLevel = 2;
   parameters->allowInstanceOverlap = FALSE;
   parameters->threshold = 0.0;
   parameters->evalMethod = EVAL_MDL;
   parameters->iterations = 1;
   strcpy(parameters->psInputFileName, "none");
   parameters->predefinedSubs = FALSE;
   parameters->minVertices = 1;
   parameters->maxVertices = 0; // i.e., infinity
   parameters->recursion = FALSE;
   parameters->variables = FALSE;
   parameters->relations = FALSE;
   parameters->incremental = FALSE;
   parameters->compress = FALSE;

   if (argc < 2)
   {
      fprintf(stderr, "input graph file name must be supplied\n");
      exit(1);
   }

    // process command-line options
   i = 1;
   while (i < (argc - 1)) 
   {
      if (strcmp(argv[i], "-beam") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: beam must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->beamWidth = ulongArg;
      } 
      else if (strcmp(argv[i], "-compress") == 0)
      {
         parameters->compress = TRUE;
      }
      else if (strcmp(argv[i], "-eval") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if ((ulongArg < 1) || (ulongArg > 3)) 
         {
            fprintf(stderr, "%s: eval must be 1-3\n", argv[0]);
            exit(1);
         }
         parameters->evalMethod = ulongArg;
      }
      else if (strcmp(argv[i], "-inc") == 0)
      {
         parameters->incremental = TRUE;
      }
      else if (strcmp(argv[i], "-iterations") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         parameters->iterations = ulongArg;
      } 
      else if (strcmp(argv[i], "-limit") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: limit must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->limit = ulongArg;
         limitSet = TRUE;
      } 
      else if (strcmp(argv[i], "-maxsize") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: maxsize must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->maxVertices = ulongArg;
      } 
      else if (strcmp(argv[i], "-minsize") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: minsize must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->minVertices = ulongArg;
      } 
      else if (strcmp(argv[i], "-nsubs") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if (ulongArg == 0) 
         {
            fprintf(stderr, "%s: nsubs must be greater than zero\n", argv[0]);
            exit(1);
         }
         parameters->numBestSubs = ulongArg;
      } 
      else if (strcmp(argv[i], "-out") == 0) 
      {
         i++;
         strcpy(parameters->outFileName, argv[i]);
         parameters->outputToFile = TRUE;
      } 
      else if (strcmp(argv[i], "-output") == 0) 
      {
         i++;
         sscanf(argv[i], "%lu", &ulongArg);
         if ((ulongArg < 1) || (ulongArg > 5)) 
         {
            fprintf(stderr, "%s: output must be 1-5\n", argv[0]);
            exit(1);
         }
         parameters->outputLevel = ulongArg;
      } 
      else if (strcmp(argv[i], "-overlap") == 0) 
      {
         parameters->allowInstanceOverlap = TRUE;
      } 
      else if (strcmp(argv[i], "-prune") == 0) 
      {
         parameters->prune = TRUE;
      } 
      else if (strcmp(argv[i], "-ps") == 0) 
      {
         i++;
         strcpy(parameters->psInputFileName, argv[i]);
         parameters->predefinedSubs = TRUE;
      } 
      else if (strcmp(argv[i], "-recursion") == 0)
      {
         parameters->recursion = TRUE;
      }
      else if (strcmp(argv[i], "-relations") == 0)
      {
         parameters->relations = TRUE;
         parameters->variables = TRUE; // relations must involve variables
      }
      else if (strcmp(argv[i], "-threshold") == 0) 
      {
         i++;
         sscanf(argv[i], "%lf", &doubleArg);
         if ((doubleArg < 0.0) || (doubleArg > 1.0)) 
         {
            fprintf(stderr, "%s: threshold must be 0.0-1.0\n", argv[0]);
            exit(1);
         }
         parameters->threshold = doubleArg;
      } 
      else if (strcmp(argv[i], "-undirected") == 0) 
      {
         parameters->directed = FALSE;
      } 
      else if (strcmp(argv[i], "-valuebased") == 0) 
      {
         parameters->valueBased = TRUE;
      } 
      else if (strcmp(argv[i], "-variables") == 0)
      {
         parameters->variables = TRUE;
      }
      else 
      {
         fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
         exit(1);
      }
      i++;
   }

   if (parameters->iterations == 0)
      parameters->iterations = MAX_UNSIGNED_LONG; // infinity

   if (parameters->incremental == TRUE)
   {
	   fprintf (stderr, "Incremental mode is not supported by the MPI version\n");
	   exit (1);
   }
   // Incremental mode does not work with MPI Subdue, but some initialization is
   // necessary to prevent seg faults
   parameters->incrementList = malloc(sizeof(IncrementList));
   parameters->incrementList->head = NULL;

   parameters->labelList = AllocateLabelList();

   // initialize log2Factorial[0..1]
   parameters->log2Factorial = (double *) malloc(2 * sizeof(double));
   if (parameters->log2Factorial == NULL)
      OutOfMemoryError("GetParameters:parameters->log2Factorial");
   parameters->log2FactorialSize = 2;
   parameters->log2Factorial[0] = 0; // lg(0!)
   parameters->log2Factorial[1] = 0; // lg(1!)

   parameters->posGraph = NULL;
   parameters->negGraph = NULL;
   parameters->numPosEgs = 0;
   parameters->numNegEgs = 0;
   parameters->posEgsVertexIndices = NULL;
   parameters->negEgsVertexIndices = NULL;

   if (processRank > 0) 
   { // child process

      parameters->numBestSubs = 1; // only care about best sub for child process
      parameters->outputLevel = 0; // no output for child process

      // read graphs from input file
      sprintf(parameters->inputFileName, "%s.part%d",
              argv[argc - 1], processRank);
      ReadInputFile(parameters);
      if (parameters->numPosEgs == 0) 
      {
         fprintf(stderr, "ERROR: no positive graphs defined\n");
         exit(1);
      }
      // check bounds on discovered substructures' number of vertices
      if (parameters->maxVertices == 0)
         parameters->maxVertices = parameters->posGraph->numVertices;
      if (parameters->maxVertices < parameters->minVertices) 
      {
         fprintf(stderr, "ERROR: minsize exceeds maxsize\n");
         exit(1);
      }
    
      // read predefined substructures
      parameters->numPreSubs = 0;
      if (parameters->predefinedSubs)
         ReadPredefinedSubsFile(parameters);
    
      parameters->posGraphDL = MDL(parameters->posGraph,
                                   parameters->labelList->numLabels,
                                   parameters);
      if (parameters->negGraph != NULL) 
      {
         parameters->negGraphDL = MDL(parameters->negGraph,
                                      parameters->labelList->numLabels,
                                      parameters);
      }

      // set limit accordingly
      if (! limitSet)
         parameters->limit = parameters->posGraph->numEdges / 2;

   } 
   else 
   { // processRank is 0 (master process)
      // record input file root
      sprintf(parameters->inputFileName, "%s", argv[argc - 1]);
      // create output file, if given
      if (parameters->outputToFile) 
      {
         outputFile = fopen(parameters->outFileName, "w");
         if (outputFile == NULL) 
         {
            printf("ERROR: unable to write to output file %s\n",
            parameters->outFileName);
            exit(1);
         }
         fclose(outputFile);
      }
   }

   return parameters;
}


//---------------------------------------------------------------------------
// NAME: PrintParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Print selected parameters.
//---------------------------------------------------------------------------

void PrintParameters(Parameters *parameters)
{
   printf("Parameters:\n");
   printf("  Number of partitions........... %lu\n",
          parameters->numPartitions);
   printf("  Input file..................... %s\n", parameters->inputFileName);
   printf("  Predefined substructure file... %s\n",
          parameters->psInputFileName);
   printf("  Output file.................... %s\n", parameters->outFileName);
   printf("  Beam width..................... %lu\n", parameters->beamWidth);
   printf("  Evaluation method.............. ");
   switch(parameters->evalMethod) 
   {
      case 1: printf("MDL\n"); break;
      case 2: printf("size\n"); break;
      case 3: printf("setcover\n"); break;
   }
   printf("  'e' edges directed............. ");
   PrintBoolean(parameters->directed);
   printf("  Iterations..................... ");
   if (parameters->iterations == 0)
      printf("infinite\n");
   else 
      printf("%lu\n", parameters->iterations);
   printf("  Limit.......................... %lu\n", parameters->limit);
   printf("  Minimum size of substructures.. %lu\n", parameters->minVertices);
   printf("  Maximum size of substructures.. %lu\n", parameters->maxVertices);
   printf("  Number of best substructures... %lu\n", parameters->numBestSubs);
   printf("  Output level................... %lu\n", parameters->outputLevel);
   printf("  Allow overlapping instances.... ");
   PrintBoolean(parameters->allowInstanceOverlap);
   printf("  Prune.......................... ");
   PrintBoolean(parameters->prune);
   printf("  Threshold...................... %lf\n", parameters->threshold);
   printf("  Value-based queue.............. ");
   PrintBoolean(parameters->valueBased);
   printf("\n");
}


//---------------------------------------------------------------------------
// NAME: FreeParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Free memory allocated for parameters.  Note that the
// predefined substructures are de-allocated as soon as they are
// processed, and not here.
//---------------------------------------------------------------------------

void FreeParameters(Parameters *parameters)
{
   FreeGraph(parameters->posGraph);
   FreeGraph(parameters->negGraph);
   FreeLabelList(parameters->labelList);
   free(parameters->posEgsVertexIndices);
   free(parameters->negEgsVertexIndices);
   free(parameters->log2Factorial);
   free(parameters);
}
