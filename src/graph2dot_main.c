//---------------------------------------------------------------------------
// graph2dot_main.c
//
// Main functions for program to convert a Subdue graph file into
// dot format as specified in AT&T Labs GraphViz package.
//
// Usage: graph2dot <graphfilename> <dotfilename>
//
// Writes the graph defined in <graphfilename> to <dotfilename>
// in dot format.
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

   if (argc != 3) 
   {
      printf("USAGE: %s <graphfilename> <dotfilename>\n", argv[0]);
      exit(1);
   }

   parameters = GetParameters(argc, argv);
   ReadInputFile(parameters);
   WriteGraphToDotFile(argv[2], parameters);

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

   // initialize parameter settings
   strcpy(parameters->inputFileName, argv[1]);
   parameters->labelList = AllocateLabelList();
   parameters->directed = TRUE;
   parameters->posGraph = NULL;
   parameters->negGraph = NULL;
   parameters->numPosEgs = 0;
   parameters->numNegEgs = 0;
   parameters->posEgsVertexIndices = NULL;
   parameters->negEgsVertexIndices = NULL;

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
   FreeGraph(parameters->posGraph);
   FreeGraph(parameters->negGraph);
   FreeLabelList(parameters->labelList);
   free(parameters->posEgsVertexIndices);
   free(parameters->negEgsVertexIndices);
   free(parameters);
}
