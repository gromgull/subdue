//---------------------------------------------------------------------------
// subs2dot_main.c
//
// Main functions for program to convert a Subdue output file into
// dot format as specified in AT&T Labs GraphViz package.
//
// Usage: subs2dot <subsfilename> <dotfilename>
//
// Writes the substructures defined in <subsfilename> to <dotfilename>
// in dot format.  Each substructure is defined as a subgraph cluster,
// and if a substructure S2 refers to a previously-discovered
// substructrure S1, then a directed edge is added from S1's cluster
// to S2's cluster.  The result is a lattice-like hierarchy of
// substructures.
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
// PURPOSE: Main function for substructure to dot conversion program.
// Takes two command-line arguments, which are the input substructures
// file and the output file for writing the dot format.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   Parameters *parameters;
   Graph **subGraphs;
   ULONG numSubGraphs;
   ULONG i;

   if (argc != 3) 
   {
      printf("USAGE: %s <subsfilename> <dotfilename>\n", argv[0]);
      exit(1);
   }

   parameters = GetParameters(argc, argv);

   // read substructures and write to dot file
   subGraphs = ReadSubGraphsFromFile(argv[1], SUB_TOKEN, &numSubGraphs,
                                     parameters);
   WriteSubsToDotFile(argv[2], subGraphs, numSubGraphs, parameters);
   printf("Wrote %lu substructures to dot file %s\n",
          numSubGraphs, argv[2]);

   // free substructure graphs
   for (i = 0; i < numSubGraphs; i++)
      FreeGraph(subGraphs[i]);
   free(subGraphs);

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

   parameters = (Parameters *) malloc(sizeof(Parameters));
   if (parameters == NULL)
      OutOfMemoryError("GetParameters:parameters");

   // initialize default parameter settings
   parameters->directed = TRUE;
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
