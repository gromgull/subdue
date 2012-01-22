//---------------------------------------------------------------------------
// test_main.c
//
// Main functions to compute FP/FN/TP/TN/Error stats for a given set of
// substructures and a given set of example graphs.
//
// Usage: test <subsfile> <graphfile>
//
// Reads in substructures from <subsfile> and then reads in example graphs
// from <graphfile> one at a time.  If any of the substructures is a
// subgraph of the example graph, that example is classified as positive,
// otherwise negative.  When done, program reports the FP/FN/TP/TN and
// error statistics.
//
// ***** NOTE: This assumes all 'e' edges are directed, and the match
// ***** threshold always equals 0, despite what parameters Subdue was run
// ***** with.
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
// PURPOSE: Main function for computing test statistics for a concept
// (disjunction of substructures) against a st of examples.  Takes two
// command-line arguments, which are the input substructures file and the
// input example graphs file.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   Parameters *parameters;
   ULONG FP = 0;
   ULONG FN = 0;
   ULONG TP = 0;
   ULONG TN = 0;
   double FPRate;
   double TPRate;
   double error;

   if (argc != 3) 
   {
      fprintf(stderr, "USAGE: %s <subsfile> <graphfile>\n", argv[0]);
      exit(1);
   }

   parameters = GetParameters(argc, argv);

   Test(argv[1], argv[2], parameters, &TP, &TN, &FP, &FN);

   // compute and output stats
   FPRate = ((double) FP) / ((double) (TN+FP));
   TPRate = ((double) TP) / ((double) (FN+TP));
   error = ((double) (FP+FN)) / ((double) (TP+TN+FP+FN));
   fprintf(stdout, "TP = %lu\nTN = %lu\nFP = %lu\nFN = %lu\n",
           TP, TN, FP, FN);
   fprintf(stdout, "(TP+FN) = %lu\n(TN+FP) = %lu\n", (TP+FN), (TN+FP));
   fprintf(stdout, "(TP+TN+FP+FN) = %lu\n", (TP+TN+FP+FN));
   fprintf(stdout, "(FP/(TN+FP)) = %f\n", FPRate);
   fprintf(stdout, "(TP/(FN+TP)) = %f\n", TPRate);
   fprintf(stdout, "Error = %f\n", error);
   fflush(stdout);

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
