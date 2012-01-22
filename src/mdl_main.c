//---------------------------------------------------------------------------
// mdl_main.c
//
// Main functions for standalone MDL computation.
//
// Usage: mdl [-dot <filename>] [-overlap] [-threshold #] g1 g2
//
// Computes the description length of g1, g2 and g2 compressed with g1
// along with the final MDL compression measure:
// dl(g2)/(dl(g1)+dl(g2|g1).  Also, prints the size-based compression
// information.  If -overlap is given, then instances of g1 may
// overlap in g2.  If -threshold is given, then instances in g2 may
// not be an exact match to g1, but the cost of transforming g1 to the
// instance is less than the threshold fraction of the size of the
// larger graph.  Default threshold is 0.0, i.e., exact match.
//
// If a filename is given with the -dot option, then the compressed
// graph is written to the file in dot format, which is defined in
// AT&T Labs GraphViz package.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


// Function prototypes

int main(int, char **);
Parameters *GetParameters(int, char **);
void FreeParameters(Parameters *);


//---------------------------------------------------------------------------
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main function for standalone subgraph isomorphism.  Takes two
// command-line arguments, which are the graph file names containing
// the graphs to be matched.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   Parameters *parameters;
   ULONG numLabels;
   ULONG subLabelIndex;
   Graph *g1;
   Graph *g2;
   Graph *g2compressed;
   InstanceList *instanceList;
   ULONG numInstances;
   ULONG subSize, graphSize, compressedSize;
   double subDL, graphDL, compressedDL;
   double value;
   Label label;

   parameters = GetParameters(argc, argv);
   g1 = ReadGraph(argv[argc - 2], parameters->labelList,
                  parameters->directed);
   g2 = ReadGraph(argv[argc - 1], parameters->labelList,
                  parameters->directed);
   instanceList = FindInstances(g1, g2, parameters);
   numInstances = CountInstances(instanceList);
   printf("Found %lu instances.\n\n", numInstances);
   g2compressed = CompressGraph(g2, instanceList, parameters);

   // Compute and print compression-based measures
   subSize = GraphSize(g1);
   graphSize = GraphSize(g2);
   compressedSize = GraphSize(g2compressed);
   value = ((double) graphSize) /
           (((double) subSize) + ((double) compressedSize));
   printf("Size of graph = %lu\n", graphSize);
   printf("Size of substructure = %lu\n", subSize);
   printf("Size of compressed graph = %lu\n", compressedSize);
   printf("Value = %f\n\n", value);

   // Compute and print MDL based measures
   numLabels = parameters->labelList->numLabels;
   subDL = MDL(g1, numLabels, parameters);
   graphDL = MDL(g2, numLabels, parameters);
   subLabelIndex = numLabels; // index of "SUB" label
   numLabels++; // add one for new "SUB" vertex label
   if ((parameters->allowInstanceOverlap) &&
       (InstancesOverlap(instanceList)))
      numLabels++; // add one for new "OVERLAP" edge label
   compressedDL = MDL(g2compressed, numLabels, parameters);
   // add extra bits to describe where external edges connect to instances
   compressedDL += ExternalEdgeBits(g2compressed, g1, subLabelIndex);
   value = graphDL / (subDL + compressedDL);
   printf("DL of graph = %f\n", graphDL);
   printf("DL of substructure = %f\n", subDL);
   printf("DL of compressed graph = %f\n", compressedDL);
   printf("Value = %f\n\n", value);

   if (parameters->outputToFile)
   {
      // first, actually add "SUB" and "OVERLAP" labels
      label.labelType = STRING_LABEL;
      label.labelValue.stringLabel = SUB_LABEL_STRING;
      StoreLabel(& label, parameters->labelList);
      label.labelValue.stringLabel = OVERLAP_LABEL_STRING;
      StoreLabel(& label, parameters->labelList);
      parameters->posGraph = g2compressed;
      WriteGraphToDotFile(parameters->outFileName, parameters);
      printf("Compressed graph written to dot file %s\n",
             parameters->outFileName);
   }

   FreeGraph(g2compressed);
   FreeInstanceList(instanceList);
   FreeGraph(g1);
   FreeGraph(g2); 
   FreeParameters(parameters);

   return 0;
}


//---------------------------------------------------------------------------
// NAME: GetParameters
//
// INPUTS: (int argc) - number of command-line arguments
//         (char *argv[]) - array of command-line argument strings
//
// RETURN: (Parameters *)
//
// PURPOSE: Initialize parameters structure and process command-line
// options.
//---------------------------------------------------------------------------

Parameters *GetParameters(int argc, char *argv[])
{
   Parameters *parameters;
   int i;
   double doubleArg;

   if (argc < 3)
   {
      fprintf(stderr, "%s: not enough arguments\n", argv[0]);
      exit(1);
   }

   parameters = (Parameters *) malloc(sizeof(Parameters));
   if (parameters == NULL)
      OutOfMemoryError("GetParameters:parameters");

   // initialize default parameter settings
   parameters->directed = TRUE;
   parameters->allowInstanceOverlap = FALSE;
   parameters->threshold = 0.0;
   parameters->outputToFile = FALSE;
   parameters->posGraph = NULL;
   parameters->negGraph = NULL;
   parameters->incremental = FALSE;

   // process command-line options
   i = 1;
   while (i < (argc - 2))
   {
      if (strcmp(argv[i], "-dot") == 0)
      {
         i++;
         strcpy(parameters->outFileName, argv[i]);
         parameters->outputToFile = TRUE;
      } 
      else if (strcmp(argv[i], "-overlap") == 0) 
      {
         parameters->allowInstanceOverlap = TRUE;
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
      else 
      {
         fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
         exit(1);
      }
      i++;
   }

   // initialize log2Factorial[0..1]
   parameters->log2Factorial = (double *) malloc(2 * sizeof(double));
   if (parameters->log2Factorial == NULL)
      OutOfMemoryError("GetParameters:paramters->log2Factorial");
   parameters->log2FactorialSize = 2;
   parameters->log2Factorial[0] = 0; // lg(0!)
   parameters->log2Factorial[1] = 0; // lg(1!)

   parameters->labelList = AllocateLabelList();

   return parameters;
}


//---------------------------------------------------------------------------
// NAME: FreeParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Free memory allocated for parameters.
//---------------------------------------------------------------------------

void FreeParameters(Parameters *parameters)
{
   FreeLabelList(parameters->labelList);
   free(parameters->log2Factorial);
   free(parameters);
}
