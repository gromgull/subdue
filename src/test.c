//---------------------------------------------------------------------------
// test.c
//
// Functions to compute FP/FN/TP/TN/Error stats for a given set of
// substructures and a given set of example graphs.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: Test
//
// INPUTS:  (char *subsFileName) - file holding substructures
//          (char *graphFileName) - file holding example graphs to test
//          (Parameters *parameters)
//          (ULONG *TPp) - number of true positives (by reference)
//          (ULONG *TNp) - number of true negatives (by reference)
//          (ULONG *FPp) - number of false positives (by reference)
//          (ULONG *FNp) - number of false negatives (by reference)
//
// RETURN:  (void)
//
// PURPOSE: Computes test statistics for a concept (disjunction of
// substructures) against a set of examples.  Takes two arguments, which
// are the input substructures file and the input example graphs file.
//---------------------------------------------------------------------------

void Test(char *subsFileName, char *graphFileName, Parameters *parameters,
          ULONG *TPp, ULONG *TNp, ULONG *FPp, ULONG *FNp)
{
   FILE *graphFile;
   LabelList *labelList;
   BOOLEAN directed;
   Graph **subGraphs;
   ULONG numSubGraphs;
   Graph *graph;
   BOOLEAN positive1;
   BOOLEAN positive2;
   ULONG vertexOffset = 0;
   ULONG lineNo = 1;
   char token[TOKEN_LEN];
   ULONG FP = 0;
   ULONG FN = 0;
   ULONG TP = 0;
   ULONG TN = 0;
   ULONG i;

   labelList = parameters->labelList;
   directed = parameters->directed;

   // read substructures
   subGraphs = ReadSubGraphsFromFile(subsFileName, SUB_TOKEN, &numSubGraphs,
                                     parameters);
   fprintf(stdout, "Read %lu substructures from file %s.\n",
           numSubGraphs, subsFileName);

   // open example graphs file and compute stats
   graphFile = fopen(graphFileName, "r");
   if (graphFile == NULL) 
   {
      fprintf(stderr, "Unable to open graph file %s.\n", graphFileName);
      exit(1);
   }

   graph = NULL;
   positive1 = TRUE;
   while (ReadToken(token, graphFile, &lineNo) != 0) 
   {
      if (strcmp(token, POS_EG_TOKEN) == 0) 
      { // reading positive eg
         if (graph != NULL) 
         {
            // test last graph
            positive2 = PositiveExample(graph, subGraphs, numSubGraphs,
                                        parameters);
            // increment appropriate counter
            if (positive1 && positive2) TP++;
            if (positive1 && (! positive2)) FN++;
            if ((! positive1) && positive2) FP++;
            if ((! positive1) && (! positive2)) TN++;
            FreeGraph(graph);
         }
         graph = AllocateGraph(0,0);
         positive1 = TRUE;
      }
      else if (strcmp(token, NEG_EG_TOKEN) == 0) 
      { // reading negative eg
         if (graph != NULL) 
         {
            // test last graph
            positive2 = PositiveExample(graph, subGraphs, numSubGraphs,
                                        parameters);
            // increment appropriate counter
            if (positive1 && positive2) TP++;
            if (positive1 && (! positive2)) FN++;
            if ((! positive1) && positive2) FP++;
            if ((! positive1) && (! positive2)) TN++;
            FreeGraph(graph);
         }
         graph = AllocateGraph(0,0);
         positive1 = FALSE;
      }
      else if (strcmp(token, "v") == 0) 
      {  // read vertex
         if (positive1 && (graph == NULL)) 
         {
            // first graph starts without positive token, so assumed positive
            graph = AllocateGraph(0,0);
         }
         ReadVertex(graph, graphFile, labelList, &lineNo, vertexOffset);
      }
      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadEdge(graph, graphFile, labelList, &lineNo, directed, vertexOffset);

      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadEdge(graph, graphFile, labelList, &lineNo, FALSE, vertexOffset);

      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadEdge(graph, graphFile, labelList, &lineNo, TRUE, vertexOffset);

      else 
      {
         fclose(graphFile);
         fprintf(stderr, "Unknown token %s in line %lu of input file %s.\n",
                 token, lineNo, graphFileName);
         exit(1);
      }
   }
   // test last graph
   if (graph != NULL) 
   {
      positive2 = PositiveExample(graph, subGraphs, numSubGraphs,
                                  parameters);
      // increment appropriate counter
      if (positive1 && positive2) TP++;
      if (positive1 && (! positive2)) FN++;
      if ((! positive1) && positive2) FP++;
      if ((! positive1) && (! positive2)) TN++;
      FreeGraph(graph);
   }

   fclose(graphFile);

   // free substructure graphs
   for (i = 0; i < numSubGraphs; i++)
      FreeGraph(subGraphs[i]);
   free(subGraphs);

   *TPp = TP;
   *TNp = TN;
   *FPp = FP;
   *FNp = FN;
}


//---------------------------------------------------------------------------
// NAME: PositiveExample
//
// INPUTS: (Graph *graph) - graph to test if positive
//         (Graph **subGraphs) - disjunctive concept of a positive example
//         (ULONG numSubGraph) - number of subgraphs
//         (Parameters *parameters)
//
// RETURN: (BOOLEAN) - TRUE is example graph is positive
//
// PURPOSE:
//---------------------------------------------------------------------------

BOOLEAN PositiveExample(Graph *graph, Graph **subGraphs,
                        ULONG numSubGraphs, Parameters *parameters)
{
   ULONG i = 0;
   BOOLEAN found = FALSE;
   InstanceList *instanceList = NULL;

   while ((i < numSubGraphs) && (! found)) 
   {
      instanceList = FindInstances(subGraphs[i], graph, parameters);
      if (instanceList->head != NULL)
         found = TRUE;
      FreeInstanceList(instanceList);
      i++;
   }
   return found;
}
