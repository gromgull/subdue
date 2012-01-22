//---------------------------------------------------------------------------
// cvtest_main.c
//
// Main functions to perform a cross-validation experiment using Subdue on
// a set of examples.
//
// Usage: cvtest [<subdue_options>] [-nfolds <n>] <graphfile>
//
// This program first divides the examples from <graphfile> into <nfolds>
// sets of examples using random selection. The program runs Subdue to find
// the substructures for the training set of each fold and then uses the
// 'test' program to evaluate the substructures on the test set of the
// fold.  The program reports the error rate for each fold and the mean
// error rate for all folds.  Note, the default for <nfolds> is 1, which
// means the entire set of examples is used for both training and testing.
// ALSO NOTE, the [-nfolds <n>] option must come after the Subdue options.
//
// This program also accepts all Subdue options (except -out), which are
// passed on to the individual calls to Subdue. See the Subdue manual for
// explanations of these options.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"

#define RM_CMD "/bin/rm -f"
#define CP_CMD "/bin/cp -f"
#define SUBDUE_PATH "subdue"


// Function prototypes

int main(int, char **);
void NumPosNegExamples(char *, ULONG *, ULONG *);
void WriteTrainTestFiles(char *, char *, char *, ULONG, ULONG *,
                         ULONG, ULONG);
void RunSubdue(int, char **, char *, char *);
void RemoveFile(char *);


//---------------------------------------------------------------------------
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main function for performing cross-validation testing. Outputs
// stats for individual folds and mean of all folds.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   Parameters *parameters;
   char inputFileName[TOKEN_LEN];
   char trainFileName[TOKEN_LEN];
   char testFileName[TOKEN_LEN];
   char subsFileName[TOKEN_LEN];
   ULONG numPosEgs;
   ULONG numNegEgs;
   ULONG numEgs;
   ULONG numFolds;
   ULONG *egFolds;
   float error;
   float meanError = 0.0;
   ULONG TP, TN, FP, FN;
   ULONG totalTP = 0, totalTN = 0, totalFP = 0, totalFN = 0;
   ULONG i;

   // Initialize parameters
   parameters = (Parameters *) malloc(sizeof(Parameters));
   if (parameters == NULL)
      OutOfMemoryError("GetParameters:parameters");
   parameters->directed = TRUE;
   parameters->labelList = AllocateLabelList();

   // Process arguments
   numFolds = 1;
   strcpy(inputFileName, argv[argc - 1]);
   argc--;

   if ((argc > 1) && (strcmp ("-nfolds", argv[argc - 2]) == 0)) 
   {
      numFolds = atol(argv[argc - 1]);
      if (numFolds < 1) 
      {
         fprintf(stderr, "-nfolds must be greater than one\n");
         exit (1);
      }
      argc -= 2;
   }
   fprintf(stdout, "Subdue Cross-Validation Testing\n");
   fprintf(stdout, "Number of Folds = %lu\n\n", numFolds);

   // Read number of positive and negative examples
   NumPosNegExamples(inputFileName, &numPosEgs, &numNegEgs);
   fprintf(stdout, "Read %lu positive and %lu negative examples.\n",
           numPosEgs, numNegEgs);
   numEgs = numPosEgs + numNegEgs;

   // Create array holding random fold number for each example
   egFolds = (ULONG *) malloc(sizeof(ULONG) * numEgs);
   if (egFolds == NULL)
      OutOfMemoryError("cvtest main: fold array");
   for (i = 0; i < numEgs; i++)
      egFolds[i] = (random() % numFolds) + 1;

   for (i = 0; i < numFolds; i++) 
   {
      fprintf (stdout, "----------\nFold %lu\n----------\n", (i+1));
      fflush (stdout);
      sprintf (trainFileName, "%s.train.%lu", inputFileName, (i+1));
      sprintf (testFileName, "%s.test.%lu", inputFileName, (i+1));
      sprintf (subsFileName, "%s.subs.%lu", inputFileName, (i+1));
      WriteTrainTestFiles(inputFileName, trainFileName, testFileName,
                          (i+1), egFolds, numEgs, numFolds);
      RunSubdue(argc, argv, trainFileName, subsFileName);
      Test(subsFileName, testFileName, parameters,
           &TP, &TN, &FP, &FN);
      error = ((double) (FP+FN)) / ((double) (TP+TN+FP+FN));
      meanError += error;
      totalTP += TP;
      totalTN += TN;
      totalFP += FP;
      totalFN += FN;
      fprintf(stdout, "Fold %lu error = %f\n", (i+1), error);
      fprintf(stdout, "  TP = %lu\n", TP);
      fprintf(stdout, "  TN = %lu\n", TN);
      fprintf(stdout, "  FP = %lu\n", FP);
      fprintf(stdout, "  FN = %lu\n\n", FN);
      RemoveFile(trainFileName);
      RemoveFile(testFileName);
      RemoveFile(subsFileName);
   }
   meanError = meanError / numFolds;
   fprintf(stdout, "%lu-fold cross validation error = %f\n",
           numFolds, meanError);
   fprintf(stdout, "  total TP = %lu\n", totalTP);
   fprintf(stdout, "  total TN = %lu\n", totalTN);
   fprintf(stdout, "  total FP = %lu\n", totalFP);
   fprintf(stdout, "  total FN = %lu\n", totalFN);
   fflush(stdout);

   FreeLabelList(parameters->labelList);
   free(parameters);

   return 0;
}


//---------------------------------------------------------------------------
// NAME: NumPosNegExamples
//
// INPUTS:  (char *fileName) - file containing example graphs
//          (ULONG *numPosEgs) - number of positive examples in file
//                               (returned by reference)
//          (ULONG *numNegEgs) - number of negative examples in file
//                               (returned by reference)
//
// RETURN:  (void)
//
// PURPOSE: Returns (by reference) the number of positive and negative
// example graphs in the given file.
//---------------------------------------------------------------------------

void NumPosNegExamples(char *fileName, ULONG *numPosEgs, ULONG *numNegEgs)
{
   FILE *fp;
   char token[TOKEN_LEN];
   ULONG lineNo = 1;
   ULONG numPos = 0;
   ULONG numNeg = 0;

   // open example graphs file and compute stats
   fp = fopen(fileName, "r");
   if (fp == NULL) 
   {
      printf("Unable to open graph file %s.\n", fileName);
      exit (1);
   }

   // get class of first graph example
   if (ReadToken(token, fp, &lineNo) == 0) 
   {
      fprintf(stderr, "No examples in graph file %s.\n", fileName);
      exit (1);
   } 
   else 
   {
      if (strcmp(token, NEG_EG_TOKEN) == 0)
         numNeg++;
      else 
         numPos++;
   }
   // scan for rest of examples
   while (fgets(token, TOKEN_LEN, fp) != NULL) 
   {
      if (strstr(token, POS_EG_TOKEN) == token)
         numPos++;
      if (strstr(token, NEG_EG_TOKEN) == token)
         numNeg++;
   }
   fclose(fp);
   *numPosEgs = numPos;
   *numNegEgs = numNeg;
}


//---------------------------------------------------------------------------
// NAME: WriteTrainTestFiles
//
// INPUTS:  (char *inputFileName) - file containing example graphs
//          (char *trainFileName) - file to hold training examples
//          (char *testFileName) - file to hold testing examples
//          (ULONG fold) - fold number for this train/test partition
//          (ULONG *egFolds) - fold assignments for examples
//          (ULONG *numEgs) - number of examples
//          (ULONG *numFolds) - number of folds
//
// RETURN:  (void)
//
// PURPOSE: Writes out examples from the input file into the training and
// testing files according to the given fold.  The egFolds array holds the
// fold assisgnment of each example.  Examples assigned to the given fold
// are written to the testing file, and the remaining examples are written
// to the training file.  If numFolds = 1, then all examples from the
// inputFile are copied to both the train and test files.
//---------------------------------------------------------------------------

void WriteTrainTestFiles(char *inputFileName, char *trainFileName,
                         char *testFileName, ULONG fold, ULONG *egFolds,
                         ULONG numEgs, ULONG numFolds) 
{
   FILE *inputFile;
   FILE *trainFile;
   FILE *testFile;
   FILE *outFile;
   char token[TOKEN_LEN];
   char command[TOKEN_LEN];
   ULONG lineNo = 1;
   ULONG i;
   BOOLEAN egIsXP = FALSE;  // true if example designated by XP
   BOOLEAN egIsXN = FALSE;  // true if example designated by XN

   if (numFolds == 1) 
   {
      sprintf(command, "%s %s %s", CP_CMD, inputFileName, trainFileName);
      system(command);
      sprintf(command, "%s %s %s", CP_CMD, inputFileName, testFileName);
      system(command);
      return;
   }

   // open input file
   inputFile = fopen(inputFileName, "r");
   if (inputFile == NULL) 
   {
      printf("Unable to open input file %s.\n", inputFileName);
      exit (1);
   }
   // get class of first graph example
   if (ReadToken(token, inputFile, &lineNo) == 0) {
      fprintf(stderr, "No examples in input file %s.\n", inputFileName);
      exit (1);
   } 
   else 
   {
      if (strcmp(token, NEG_EG_TOKEN) == 0)
         egIsXN = TRUE;
      if (strcmp(token, POS_EG_TOKEN) == 0)
         egIsXP = TRUE;
   }

   // open train and test files
   trainFile = fopen(trainFileName, "w");
   if (trainFile == NULL) 
   {
      printf("Unable to open train file %s.\n", trainFileName);
      exit (1);
   }
   testFile = fopen(testFileName, "w");
   if (testFile == NULL) 
   {
      printf("Unable to open test file %s.\n", testFileName);
      exit (1);
   }

   // write examples to train and test files
   for (i = 0; i < numEgs; i++) 
   {
      if (egFolds[i] == fold)
         outFile = testFile;
      else 
         outFile = trainFile;
      if (egIsXN == TRUE) 
      {
         fprintf(outFile, "%s\n", NEG_EG_TOKEN);
         egIsXN = FALSE;
      } 
      else 
      {
         fprintf(outFile, "%s\n", POS_EG_TOKEN);
         egIsXP = FALSE;
      }
      // output rest of example
      while ((! egIsXP) && (! egIsXN) &&
             (fgets(token, TOKEN_LEN, inputFile) != NULL)) 
      {
         if (strstr(token, POS_EG_TOKEN) == token)
            egIsXP = TRUE;
         else if (strstr(token, NEG_EG_TOKEN) == token)
            egIsXN = TRUE;
         else if (token[0] != COMMENT) // strip out full-line comments
            fprintf(outFile, "%s", token);
      }
   }
   fclose(testFile);
   fclose(trainFile);
   fclose(inputFile);
}


//---------------------------------------------------------------------------
// NAME: RunSubdue
//
// INPUTS:  (int argc) - number of arguments for Subdue
//          (char **argv) - Subdue arguments (argv[1...argc-1])
//          (char *trainFileName) - input file for Subdue run
//          (char *subsFileName) - substructure output file for Subdue run
//
// RETURN:  (void)
//
// PURPOSE: Constructs an appropriate Subdue command line and then executes
// it.
//---------------------------------------------------------------------------

void RunSubdue(int argc, char **argv, char *trainFileName,
               char *subsFileName)
{
   char command[TOKEN_LEN];
   int i;
   int status;

   strcpy(command, SUBDUE_PATH);
   // add Subdue arguments passed to cvtest
   for (i = 1; i < argc; i++) 
   {
      strcat(command, " ");
      strcat(command, argv[i]);
   }
   // append substructure output file option
   strcat(command, " -out ");
   strcat(command, subsFileName);
   // append input file name
   strcat(command, " ");
   strcat(command, trainFileName);

   status = system(command);
   if (status != 0) 
   {
      fprintf(stderr, "ERROR: unable to run command: %s\n",command);
      exit (1);
   }
}


//---------------------------------------------------------------------------
// NAME: RemoveFile
//
// INPUTS:  (char *fileName) - name of file to be removed
//
// RETURN:  (void)
//
// PURPOSE: Deletes the given file.
//---------------------------------------------------------------------------

void RemoveFile(char *fileName)
{
   char command[TOKEN_LEN];

   sprintf(command, "%s %s", RM_CMD, fileName);
   system(command);
}
