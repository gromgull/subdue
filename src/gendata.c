//---------------------------------------------------------------------------
// gendata.c
//
// Generate data for iSubdue
//
// Subdue 5
//
// Independent Data:  Increments are completely separate
// Connected Data:    Edges span increment boundaries
//
// Generates a specified number of increments and writes each one to a separate
// file, which is named according to its increment number.
//
// Increments are comprised of random graph data, which is then seeded with 
// predefined substructures based on some density.  For connected data, the 
// random data is connected across some increment boundary and a few of the
// predefined substructures are also connected across the increment boundary
//
// By going to a file-based system we broke graph compression.  We need to set 
// up a process to remap the vertex numbers in successive increments if we 
// compress an increment.  TBD
//---------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "subdue.h"

//globals for this module only
static int incrementCount = 1;
static BOOLEAN start = TRUE;
static BOOLEAN readingPositive = TRUE;
ULONG numIncrementPosVertices;
ULONG numIncrementPosEdges;
ULONG numIncrementNegVertices;
ULONG numIncrementNegEdges;

//---------------------------------------------------------------------------
// NAME: GetNextIncrement
//
// INPUTS:  (Parameters *parameters)
//
// RETURN:  BOOLEAN
//
// PURPOSE: Update the posGraph held in the parameters with the next data 
//          increment.  Add a new Increment structure to the list,
//	    representing the new data increment.
//---------------------------------------------------------------------------

BOOLEAN GetNextIncrement(Parameters *parameters) 
{
   long ltime;
   int stime;
   BOOLEAN newData;
   ULONG startPosVertexIndex = 0;
   ULONG startPosEdgeIndex = 0;
   ULONG startNegVertexIndex = 0;
   ULONG startNegEdgeIndex = 0;

   // If this is the first increment then set up the graph structure info
   if (start)
   {
      InitializeGraph(parameters);
      start = FALSE;
   }

   startPosVertexIndex = parameters->posGraph->numVertices;  // total vertices
   if (parameters->negGraph != NULL)
      startNegVertexIndex = parameters->negGraph->numVertices;

   startNegEdgeIndex = parameters->posGraph->numEdges;  // total edges
   if (parameters->negGraph != NULL)
      startNegEdgeIndex = parameters->negGraph->numEdges;

   numIncrementPosVertices = 0;    // counter for num vertices in this increment
   numIncrementPosEdges = 0;       // counter for num edges in this increment
   numIncrementNegVertices = 0;    // counter for num vertices in this increment
   numIncrementNegEdges = 0;       // counter for num edges in this increment

   ltime = time(NULL);
   stime = (unsigned) ltime/2;
   srand(stime);

   newData = CreateFromFile(parameters,
                            startPosVertexIndex, startNegVertexIndex);
   AddNewIncrement(startPosVertexIndex, startPosEdgeIndex,
                   startNegVertexIndex, startNegEdgeIndex,
                   numIncrementPosVertices, numIncrementPosEdges,
                   numIncrementNegVertices, numIncrementNegEdges, parameters);
   incrementCount++;

   return newData;
}


//----------------------------------------------------------------------------
// NAME:  InitializeGraph
//
// INPUTS:  (Parameters *parameters)
//
// RETURN:  void
//
// PURPOSE:  Initialize graph variables.
//----------------------------------------------------------------------------

void InitializeGraph(Parameters *parameters)
{
   parameters->posGraph = AllocateGraph(0,0);
   parameters->negGraph = NULL;
   parameters->numPosEgs = 0;
   parameters->numNegEgs = 0;
   parameters->posEgsVertexIndices = NULL;
   parameters->negEgsVertexIndices = NULL;
}


//----------------------------------------------------------------------------
// NAME:  CreateFromFile
//
// INPUTS:  (Parameters *parameters)
//          (ULONG startPosVertexIndex)
//          (ULONG startNegVertexIndex)
//
// RETURN:  BOOLEAN
//
// PURPOSE:  Read data for the next increment.
//----------------------------------------------------------------------------

BOOLEAN CreateFromFile(Parameters *parameters,
                       ULONG startPosVertexIndex, ULONG startNegVertexIndex)
{
   char fileName[50];
   char str[20];
   BOOLEAN newData = FALSE;

   sprintf(str, "_%d", incrementCount);
   strcpy(fileName, parameters->inputFileName);
   strcat(fileName, str);
   strcat(fileName, ".g");

   newData = ReadIncrement(fileName, parameters,
                           startPosVertexIndex, startNegVertexIndex);
   return newData;	
}


//-------------------------------------------------------------------------
// NAME:  ReadIncrement
//
// INPUTS:  (char *filename) - filename for next increment
//          (Parameters *parameters)
//          (ULONG startVertex)
//
// RETURN:  BOOLEAN
//
// PURPOSE:  Read graph data from the file for the next increment.
//-------------------------------------------------------------------------

BOOLEAN ReadIncrement(char *filename, Parameters *parameters,
                      ULONG startPosVertex, ULONG startNegVertex)
{
   Graph *posGraph;
   Graph *negGraph;
   Graph *graph;
   FILE *graphFile;
   LabelList *labelList;
   char token[TOKEN_LEN];
   ULONG lineNo;             // Line number counter for graph file
   ULONG *posVertexIndices = parameters->posEgsVertexIndices;
   ULONG *negVertexIndices = parameters->negEgsVertexIndices;
   ULONG numPosExamples = parameters->numPosEgs;
   ULONG numNegExamples = parameters->numNegEgs;
   ULONG vertexOffset = 0;
   ULONG startVertex=0;
   BOOLEAN directed;
   BOOLEAN newData;

   labelList = parameters->labelList;
   directed = parameters->directed;
   posGraph = parameters->posGraph;
   negGraph = parameters->negGraph;
   graph = posGraph;
   startVertex = startPosVertex;
 
   // Open graph file
   graphFile = fopen(filename,"r");
   if (graphFile == NULL)
   {
      printf("End of Input.\n");
      newData = FALSE;
      return newData;
   }
   else 
      newData = TRUE;

   // Parse graph file
   lineNo = 1;
   while (ReadToken(token, graphFile, &lineNo) != 0)
   {
      if (strcmp(token, POS_EG_TOKEN) == 0)        // Read positive example
      {
         numPosExamples++;
         vertexOffset = posGraph->numVertices;
         posVertexIndices = AddVertexIndex(posVertexIndices, numPosExamples, vertexOffset);
         graph = posGraph;
         startVertex = startPosVertex;
         readingPositive = TRUE;
      }
      else if (strcmp(token, NEG_EG_TOKEN) == 0)   // Read negative example
      {
         if (negGraph == NULL)
         {
            parameters->negGraph = AllocateGraph(0,0);
            negGraph = parameters->negGraph;
         }
         numNegExamples++;
         vertexOffset = negGraph->numVertices;
         negVertexIndices = AddVertexIndex(negVertexIndices, numNegExamples, vertexOffset);
         graph = negGraph;
         startVertex = startNegVertex;
         readingPositive = FALSE;
      }
      else if (strcmp(token, "v") == 0)         // read vertex
      {
         if (readingPositive && numPosExamples == 0)
         {
            numPosExamples++;
            vertexOffset = posGraph->numVertices;
            posVertexIndices = AddVertexIndex(posVertexIndices, numPosExamples, vertexOffset);
            graph = posGraph;
            startVertex = startPosVertex;
	 }
         ReadIncrementVertex(graph, graphFile, labelList, &lineNo, vertexOffset);
      }
      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadIncrementEdge(graph, graphFile, labelList, &lineNo, directed, startVertex, vertexOffset);
      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadIncrementEdge(graph, graphFile, labelList, &lineNo, FALSE, startVertex, vertexOffset);
      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadIncrementEdge(graph, graphFile, labelList, &lineNo, TRUE, startVertex, vertexOffset);
      else
      {
         fclose(graphFile);
         FreeGraph(posGraph);
         FreeGraph(negGraph);
         fprintf(stderr, "Unknown token %s in line %lu of graph file %s.\n",
                    token, lineNo, filename);
         exit(1);
      }
   }
   fclose(graphFile);
   parameters->numPosEgs = numPosExamples;
   parameters->numNegEgs = numNegExamples;
   parameters->posEgsVertexIndices = posVertexIndices;
   parameters->negEgsVertexIndices = negVertexIndices;

   // trim vertex, edge and label lists
   return newData;
}


//--------------------------------------------------------------
// NAME:  ReadIncrementVertex
//
// INPUTS: (Graph *graph)
//         (FILE *fp)
//         (LabelList *labelList)
//         (ULONG *pLineNo)
//         (ULONG vertexOffset)
//
// RETURN:  void
//
// PURPOSE:  Read information for a single vertex in the current
//           increment file.
//--------------------------------------------------------------

void ReadIncrementVertex(Graph *graph, FILE *fp, LabelList *labelList,
                         ULONG *pLineNo, ULONG vertexOffset)
{
   ULONG vertexID;
   ULONG labelIndex;

   // read and check vertex number
   vertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (vertexID != (graph->numVertices + 1))
   {
      fprintf(stderr, "Error: invalid vertex number at line %lu.\n", *pLineNo);
      exit(1);
   }
   // read label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   AddVertex(graph, labelIndex);
   if (readingPositive)
      numIncrementPosVertices++;
   else 
      numIncrementNegVertices++;
}


//--------------------------------------------------------------
// NAME:  ReadIncrementEdge
//
// INPUTS: (Graph *graph)
//         (FILE *fp)
//         (LabelList *labelList)
//         (ULONG *pLineNo)
//         (BOOLEAN directed)
//         (ULONG startVertexIndex)
//         (ULONG vertexOffset)
//
// RETURN:  void
//
// PURPOSE:  Read information for a single edge in the current
//           increment file.
//--------------------------------------------------------------

void ReadIncrementEdge(Graph *graph, FILE *fp, LabelList *labelList,
                       ULONG *pLineNo, BOOLEAN directed,
                       ULONG startVertexIndex, ULONG vertexOffset)
{
   ULONG sourceVertexID;
   ULONG targetVertexID;
   ULONG sourceVertexIndex;
   ULONG targetVertexIndex;
   ULONG labelIndex;
   BOOLEAN spansIncrement;

   // read and check vertex numbers
   sourceVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (sourceVertexID > graph->numVertices)
   {
      fprintf(stderr,
        "Error: reference to undefined vertex number at line %lu.\n", *pLineNo);
      exit(1);
   }
   targetVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (targetVertexID > graph->numVertices)
   {
      fprintf(stderr,
        "Error: reference to undefined vertex number at line %lu.\n", *pLineNo);
      exit(1);
   }
   sourceVertexIndex = sourceVertexID - 1;
   targetVertexIndex = targetVertexID - 1;

   // read and store label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   if ((sourceVertexIndex < startVertexIndex) ||
       (targetVertexIndex < startVertexIndex))
      spansIncrement = TRUE;
   else 
      spansIncrement = FALSE;

   AddEdge(graph, sourceVertexIndex, targetVertexIndex, directed, labelIndex, spansIncrement);

   if (readingPositive)
      numIncrementPosEdges++;
   else 
      numIncrementNegEdges++;
}
