//---------------------------------------------------------------------------
// sgiso_main.c
//
// Main functions for standalone subgraph isomorphism algorithm.
//
// Usage: sgiso [-dot <filename>] [-overlap] [-threshold #] g1 g2
//
// Finds and prints all instances of g1 in g2.  If -overlap is given,
// then instances may overlap in g2.  If -threshold is given, then
// instances may not be an exact match to g1, but the cost of
// transforming g1 to the instance is less than the threshold fraction
// of the size of the larger graph.  Default threshold is 0.0, i.e.,
// exact match.
//
// If a filename is given with the -dot option, then g2 is written to
// the file in dot format, with instances highlighted in red, which is
// defined in AT&T Labs GraphViz package.
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
   Graph *g1;
   Graph *g2;
   InstanceList *instanceList;
   ULONG numInstances;

   parameters = GetParameters(argc, argv);
   g1 = ReadGraph(argv[argc - 2], parameters->labelList,
                  parameters->directed);
   g2 = ReadGraph(argv[argc - 1], parameters->labelList,
                  parameters->directed);
   instanceList = FindInstances(g1, g2, parameters);
   numInstances = CountInstances(instanceList);
   PrintInstanceList(instanceList, g2, parameters->labelList);
   printf("\nFound %lu instances.\n", numInstances);

   // write graph g2 with instances to dot file, if requested
   if (parameters->outputToFile) 
   {
      WriteGraphWithInstancesToDotFile(parameters->outFileName,
                                       g2, instanceList, parameters);
      printf("\nGraph with instances written to dot file %s.\n",
             parameters->outFileName);
   }
  
   FreeInstanceList(instanceList);
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
   free(parameters);
}
