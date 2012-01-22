//---------------------------------------------------------------------------
// graphops.c
//
// Graph allocation, deallocation, input and output functions.
//
// SUBDUE 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: ReadInputFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in the SUBDUE input file, which may consist of
// positive graphs and/or negative graphs, which are collected into
// the positive and negative graph fields of the parameters.  Each
// example in the input file is prefaced by the appropriate token defined
// in subdue.h.  The first graph in the file is assumed positive
// unless the negative token is present.  Each graph is assumed to
// begin at vertex #1 and therefore examples are not connected to one
// another.
//---------------------------------------------------------------------------

void ReadInputFile(Parameters *parameters)
{
   FILE *inputFile = NULL;
   Graph *graph = NULL;
   Graph *posGraph= NULL;
   Graph *negGraph = NULL;
   LabelList *labelList = NULL;
   ULONG numPosEgs = 0;
   ULONG numNegEgs = 0;
   ULONG *posEgsVertexIndices = NULL;
   ULONG *negEgsVertexIndices = NULL;
   BOOLEAN readingPositive = TRUE;
   ULONG vertexOffset = 0;
   BOOLEAN directed = TRUE;
   ULONG lineNo = 1;
   char token[TOKEN_LEN];

   labelList = parameters->labelList;
   directed = parameters->directed;

   // Open input file
   inputFile = fopen(parameters->inputFileName,"r");
   if (inputFile == NULL) 
   {
      fprintf(stderr, "Unable to open input file %s.\n",
              parameters->inputFileName);
      exit(1);
   }

   // Parse input file
   while (ReadToken(token, inputFile, &lineNo) != 0) 
   {
      if (strcmp(token, POS_EG_TOKEN) == 0) 
      { // reading positive eg
         if (posGraph == NULL)
            posGraph = AllocateGraph(0,0);
         numPosEgs++;
         vertexOffset = posGraph->numVertices;
         posEgsVertexIndices = AddVertexIndex(posEgsVertexIndices, numPosEgs, vertexOffset);
         graph = posGraph;
         readingPositive = TRUE;
      }
      else if (strcmp(token, NEG_EG_TOKEN) == 0) 
      { // reading negative eg
         if (negGraph == NULL)
            negGraph = AllocateGraph(0,0);
         numNegEgs++;
         vertexOffset = negGraph->numVertices;
         negEgsVertexIndices = AddVertexIndex(negEgsVertexIndices,
                                              numNegEgs, vertexOffset);
         graph = negGraph;
         readingPositive = FALSE;
      }
      else if (strcmp(token, "v") == 0) 
      {  // read vertex
         if (readingPositive && (posGraph == NULL)) 
         {
            // first graph starts without positive token, so assumed positive
            posGraph = AllocateGraph(0,0);
            numPosEgs++;
            vertexOffset = 0;
            posEgsVertexIndices = AddVertexIndex(posEgsVertexIndices, numPosEgs, vertexOffset);
            graph = posGraph;
         }
         ReadVertex(graph, inputFile, labelList, &lineNo, vertexOffset);
      }
      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadEdge(graph, inputFile, labelList, &lineNo, directed, vertexOffset);

      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadEdge(graph, inputFile, labelList, &lineNo, FALSE, vertexOffset);

      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadEdge(graph, inputFile, labelList, &lineNo, TRUE, vertexOffset);

      else 
      {
         fclose(inputFile);
         fprintf(stderr, "Unknown token %s in line %lu of input file %s.\n",
                 token, lineNo, parameters->inputFileName);
         exit(1);
      }
   }
   fclose(inputFile);

   //***** trim vertex, edge and label lists

   parameters->posGraph = posGraph;
   parameters->negGraph = negGraph;
   parameters->labelList = labelList;
   parameters->numPosEgs = numPosEgs;
   parameters->numNegEgs = numNegEgs;
   parameters->posEgsVertexIndices = posEgsVertexIndices;
   parameters->negEgsVertexIndices = negEgsVertexIndices;
}


//---------------------------------------------------------------------------
// NAME: AddVertexIndex
//
// INPUTS: (ULONG *vertexIndices) - array of indices to augment
//         (ULONG n) - size of new array
//         (ULONG index) - index to add to nth element of array
//
// RETURN: (ULONG *) - new vertex index array
//
// PURPOSE: Reallocate the given vertex index array and store the new
// index in the nth element of the array.  This is used to build the
// array of indices into the positive and negative examples graphs.
//---------------------------------------------------------------------------

ULONG *AddVertexIndex(ULONG *vertexIndices, ULONG n, ULONG index)
{
   vertexIndices = (ULONG *) realloc(vertexIndices, sizeof(ULONG) * n);
   if (vertexIndices == NULL)
      OutOfMemoryError("AddVertexIndex:vertexIndices");
   vertexIndices[n - 1] = index;
   return vertexIndices;
}


//---------------------------------------------------------------------------
// NAME: ReadPredefinedSubsFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in one or more graphs from the given file and stores
// these on the predefined substructure list in parameters.  Each
// graph is prefaced by the predefined substructure token defined in
// subdue.h.
//
// Right now, these substructures will be used to compress the graph,
// if present, and therefore any labels not present in the input graph
// will be discarded during compression.  If the predefined
// substructures are ever simply put on the discovery queue, then care
// should be taken to not include labels that do not appear in the
// input graph, as this would bias the MDL computation. (*****)
//---------------------------------------------------------------------------

void ReadPredefinedSubsFile(Parameters *parameters)
{
   ULONG numPreSubs = 0;
   Graph **preSubs = NULL;

   preSubs = ReadSubGraphsFromFile(parameters->psInputFileName,
                                   PREDEF_SUB_TOKEN,
                                   & numPreSubs,
                                   parameters);
   parameters->numPreSubs = numPreSubs;
   parameters->preSubs = preSubs;
}


//---------------------------------------------------------------------------
// NAME: ReadSubGraphsFromFile
//
// INPUTS: (char *fileName) - file containing substructure graphs
//         (char *subToken) - token used to separate sub graphs in file
//         (ULONG *numSubGraphs) - call-by-reference variable returning
//                                 number of substructure graphs read
//         (Parameters *parameters)
//
// RETURN: (Graph **) - array of substructure graphs
//
// PURPOSE: Reads in one or more graphs from the given file and stores
// them in an array of graphs.  Each graph (except optionally the
// first) is assumed to be prefaced by the given substructure token.
//---------------------------------------------------------------------------

Graph **ReadSubGraphsFromFile(char *fileName, char *subToken,
                              ULONG *numSubGraphs, Parameters *parameters)
{
   ULONG numSubs = 0;
   Graph **subGraphs = NULL;
   FILE *inputFile = NULL;
   Graph *graph = NULL;
   LabelList *labelList = NULL;
   BOOLEAN directed = TRUE;
   ULONG vertexOffset = 0;   // Dummy argument to ReadVertex and ReadEdge
   ULONG lineNo = 1;
   char token[TOKEN_LEN];

   labelList = parameters->labelList;
   directed = parameters->directed;

   // Open input file
   inputFile = fopen(fileName,"r");
   if (inputFile == NULL) 
   {
      fprintf(stderr, "Unable to open input file %s.\n", fileName);
      exit(1);
   }

   // Parse input file
   while (ReadToken(token, inputFile, &lineNo) != 0) 
   {
      if (strcmp(token, subToken) == 0) 
      { // new sub-graph
         numSubs++;
         subGraphs = (Graph **) realloc(subGraphs, (sizeof(Graph *) * numSubs));
         if (subGraphs == NULL)
            OutOfMemoryError("ReadSubGraphsFromFile:subGraphs");
         subGraphs[numSubs - 1] = AllocateGraph(0, 0);
         graph = subGraphs[numSubs - 1];
       }
      else if (strcmp(token, "v") == 0) 
      {        // read vertex
         if (subGraphs == NULL) 
         {
            // first sub-graph not preceded by subToken
            numSubs++;
            subGraphs = (Graph **) realloc(subGraphs,
                                           (sizeof(Graph *) * numSubs));
            if (subGraphs == NULL)
               OutOfMemoryError("ReadSubGraphsFromFile:subGraphs");
            subGraphs[numSubs - 1] = AllocateGraph(0, 0);
            graph = subGraphs[numSubs - 1];
          }
         ReadVertex(graph, inputFile, labelList, &lineNo, vertexOffset);
      }
      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadEdge(graph, inputFile, labelList, &lineNo, directed, vertexOffset);

      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadEdge(graph, inputFile, labelList, &lineNo, FALSE, vertexOffset);

      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadEdge(graph, inputFile, labelList, &lineNo, TRUE, vertexOffset);

      else 
      {
         fclose(inputFile);
         fprintf(stderr, "Unknown token %s in line %lu of input file %s.\n",
                 token, lineNo, fileName);
         exit(1);
      }
   }
   fclose(inputFile);

   //***** trim vertex, edge and label lists

   *numSubGraphs = numSubs;
   return subGraphs;
}


//---------------------------------------------------------------------------
// NAME:    ReadGraph
//
// INPUTS:  (char *filename) - graph file to read from
//          (LabelList *labelList) - list of labels to be added to from graph
//          (BOOLEAN directed) - TRUE if 'e' edges should be directed
//
// RETURN:  (Graph *) - graph read from file
//
// PURPOSE: Parses graph file, checking for formatting errors, and builds
// all necessary structures for the graph, which is returned.  labelList
// is destructively changed to hold any new labels.
//---------------------------------------------------------------------------

Graph *ReadGraph(char *filename, LabelList *labelList, BOOLEAN directed)
{
   Graph *graph;
   FILE *graphFile;
   ULONG lineNo;             // Line number counter for graph file
   char token[TOKEN_LEN];
   ULONG vertexOffset = 0;   // Dummy argument to ReadVertex and ReadEdge

   // Allocate graph
   graph = AllocateGraph(0,0);

   // Open graph file
   graphFile = fopen(filename,"r");
   if (graphFile == NULL) 
   {
      fprintf(stderr, "Unable to open graph file %s.\n", filename);
      exit(1);
   }

   // Parse graph file
   lineNo = 1;
   while (ReadToken(token, graphFile, &lineNo) != 0) 
   {
      if (strcmp(token, "v") == 0)         // read vertex
         ReadVertex(graph, graphFile, labelList, &lineNo, vertexOffset);

      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadEdge(graph, graphFile, labelList, &lineNo, directed, vertexOffset);

      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadEdge(graph, graphFile, labelList, &lineNo, FALSE, vertexOffset);

      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadEdge(graph, graphFile, labelList, &lineNo, TRUE, vertexOffset);

      else 
      {
         fclose(graphFile);
         FreeGraph(graph);
         fprintf(stderr, "Unknown token %s in line %lu of graph file %s.\n",
                 token, lineNo, filename);
         exit(1);
      }
   }
   fclose(graphFile);

   //***** trim vertex, edge and label lists

   return graph;
}


//---------------------------------------------------------------------------
// NAME:    ReadVertex
//
// INPUTS:  (Graph *graph) - pointer to graph being constructed
//          (FILE *fp) - pointer to graph file stream
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *pLineNo) - pointer to line counter in calling function
//          (ULONG vertexOffset) - offset to add to vertex numbers
//
// RETURN:  (void)
//
// PURPOSE: Read and check the vertex number and label, store label in
// given label list, and add vertex to graph.  A non-zero vertexOffset
// indicates this vertex is part of a graph beyond the first.
//--------------------------------------------------------------------------

void ReadVertex(Graph *graph, FILE *fp, LabelList *labelList, ULONG *pLineNo, ULONG vertexOffset)
{
   ULONG vertexID;
   ULONG labelIndex;

   // read and check vertex number
   vertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (vertexID != (graph->numVertices + 1)) 
   {
      fprintf(stderr, "Error: invalid vertex number at line %lu.\n",
              *pLineNo);
      exit(1);
   }
   // read label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   AddVertex(graph, labelIndex);
}


//---------------------------------------------------------------------------
// NAME: AddVertex
//
// INPUTS: (Graph *graph) - graph to add vertex
//         (ULONG labelIndex) - index into label list of vertex's label
//
// RETURN: (void)
//
// PURPOSE: Add vertex information to graph. AddVertex also changes the
// size of the currently-allocated vertex array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//---------------------------------------------------------------------------

void AddVertex(Graph *graph, ULONG labelIndex)
{
   Vertex *newVertexList;
   ULONG numVertices = graph->numVertices;
   ULONG vertexListSize = graph->vertexListSize;

   // make sure there is enough room for another vertex
   if (vertexListSize == numVertices)
   {
      vertexListSize += LIST_SIZE_INC;
      newVertexList = (Vertex *) realloc(graph->vertices, (sizeof(Vertex) * (vertexListSize)));
      if (newVertexList == NULL)
         OutOfMemoryError("vertex list");
      graph->vertices = newVertexList;
      graph->vertexListSize = vertexListSize;
   }

   // store information in vertex
   graph->vertices[numVertices].label = labelIndex;
   graph->vertices[numVertices].numEdges = 0;
   graph->vertices[numVertices].edges = NULL;
   graph->vertices[numVertices].map = VERTEX_UNMAPPED;
   graph->vertices[numVertices].used = FALSE;
   graph->numVertices++;
}


//---------------------------------------------------------------------------
// NAME:    ReadEdge
//
// INPUTS:  (Graph *graph) - pointer to graph being constructed
//          (FILE *fp) - pointer to graph file stream
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *pLineNo) - pointer to line counter in calling function
//          (BOOLEAN directed) - TRUE if edge is directed
//          (ULONG vertexOffset) - offset to add to vertex numbers
//
// RETURN:  (void)
//
// PURPOSE: Read and check the vertex numbers and label, store label in
// given label list, and add edge to graph.  A non-zero vertexOffset
// indicates the edge's vertices are part of a graph beyond the first.
//---------------------------------------------------------------------------

void ReadEdge(Graph *graph, FILE *fp, LabelList *labelList, ULONG *pLineNo, BOOLEAN directed, ULONG vertexOffset)
{
   ULONG sourceVertexID;
   ULONG targetVertexID;
   ULONG sourceVertexIndex;
   ULONG targetVertexIndex;
   ULONG labelIndex;

   // read and check vertex numbers
   sourceVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if ((sourceVertexID < 1) || (sourceVertexID > graph->numVertices))
   {
      fprintf(stderr,
              "Error: reference to undefined vertex number at line %lu.\n", 
              *pLineNo);
      exit(1);
   }
   targetVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if ((targetVertexID < 1) || (targetVertexID > graph->numVertices))
   {
      fprintf(stderr,
              "Error: reference to undefined vertex number at line %lu.\n", 
              *pLineNo);
      exit(1);
   }
   sourceVertexIndex = sourceVertexID - 1;
   targetVertexIndex = targetVertexID - 1;

   // read and store label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   AddEdge(graph, sourceVertexIndex, targetVertexIndex, directed, labelIndex, FALSE);
}


//---------------------------------------------------------------------------
// NAME: AddEdge
//
// INPUTS: (Graph *graph) - graph to add edge to
//         (ULONG sourceVertexIndex) - index of edge's source vertex
//         (ULONG targetVertexIndex) - index of edge's target vertex
//         (BOOLEAN directed) - TRUE is edge is directed
//         (ULONG labelIndex) - index of edge's label in label list
//         (ULONG spansIncrement)
//
// RETURN: (void)
//
// PURPOSE: Add edge information to graph. AddEdge also changes the
// size of the currently-allocated edge array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//---------------------------------------------------------------------------

void AddEdge(Graph *graph, ULONG sourceVertexIndex, ULONG targetVertexIndex,
             BOOLEAN directed, ULONG labelIndex, BOOLEAN spansIncrement)
{
   Edge *newEdgeList;
   ULONG edgeListSize = graph->edgeListSize;

   // make sure there is enough room for another edge in the graph
   if (edgeListSize == graph->numEdges)
   {
      edgeListSize += LIST_SIZE_INC;
      newEdgeList = (Edge *) realloc(graph->edges, (sizeof(Edge) * (edgeListSize)));
      if (newEdgeList == NULL)
         OutOfMemoryError("AddEdge:newEdgeList");
      graph->edges = newEdgeList;
      graph->edgeListSize = edgeListSize;
   }

   // add edge to graph
   graph->edges[graph->numEdges].vertex1 = sourceVertexIndex;
   graph->edges[graph->numEdges].vertex2 = targetVertexIndex;
   graph->edges[graph->numEdges].label = labelIndex;
   graph->edges[graph->numEdges].directed = directed;
   graph->edges[graph->numEdges].used = FALSE;
   graph->edges[graph->numEdges].spansIncrement = spansIncrement;
   graph->edges[graph->numEdges].validPath = TRUE;

   // add index to edge in edge index array of both vertices
   AddEdgeToVertices(graph, graph->numEdges);

   graph->numEdges++;
}


//---------------------------------------------------------------------------
// NAME: StoreEdge
//
// INPUTS: (Edge *overlapEdges) - edge array where edge is stored
//         (ULONG edgeIndex) - index into edge array where edge is stored
//         (ULONG v1) - vertex1 of edge
//         (ULONG v2) - vertex2 of edge
//         (ULONG label) - edge label index
//         (BOOLEAN directed) - edge directedness
//         (BOOLEAN spansIncrement) - edge crossing increment boundary
//
// RETURN: (void)
//
// PURPOSE: Procedure to store an edge in given edge array.
//---------------------------------------------------------------------------

void StoreEdge(Edge *overlapEdges, ULONG edgeIndex,
               ULONG v1, ULONG v2, ULONG label, BOOLEAN directed, 
               BOOLEAN spansIncrement)
{
   overlapEdges[edgeIndex].vertex1 = v1;
   overlapEdges[edgeIndex].vertex2 = v2;
   overlapEdges[edgeIndex].label = label;
   overlapEdges[edgeIndex].directed = directed;
   overlapEdges[edgeIndex].used = FALSE;
   overlapEdges[edgeIndex].spansIncrement = spansIncrement;
}


//---------------------------------------------------------------------------
// NAME: AddEdgeToVertices
//
// INPUTS: (Graph *graph) - graph containing edge and vertices
//         (ULONG edgeIndex) - edge's index into graph edge array
//
// RETURN: (void)
//
// PURPOSE: Add edge index to the edge array of each of the two
// vertices involved in the edge.  If a self-edge, then only add once.
//---------------------------------------------------------------------------

void AddEdgeToVertices(Graph *graph, ULONG edgeIndex)
{
   ULONG v1, v2;
   Vertex *vertex;
   ULONG *edgeIndices;

   v1 = graph->edges[edgeIndex].vertex1;
   v2 = graph->edges[edgeIndex].vertex2;
   vertex = & graph->vertices[v1];
   edgeIndices = (ULONG *) realloc(vertex->edges,
                                   sizeof(ULONG) * (vertex->numEdges + 1));
   if (edgeIndices == NULL)
      OutOfMemoryError("AddEdgeToVertices:edgeIndices1");
   edgeIndices[vertex->numEdges] = edgeIndex;
   vertex->edges = edgeIndices;
   vertex->numEdges++;

   if (v1 != v2) 
   { // don't add a self edge twice
      vertex = & graph->vertices[v2];
      edgeIndices = (ULONG *) realloc(vertex->edges,
                                      sizeof(ULONG) * (vertex->numEdges + 1));
      if (edgeIndices == NULL)
         OutOfMemoryError("AddEdgeToVertices:edgeIndices2");
      edgeIndices[vertex->numEdges] = edgeIndex;
      vertex->edges = edgeIndices;
      vertex->numEdges++;
   }
}


//---------------------------------------------------------------------------
// NAME:    ReadLabel
//
// INPUTS:  (FILE *fp) - file pointer from which label is read
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (ULONG) - index of read label in global label list.
//
// PURPOSE: Reads a label (string or numeric) from the given file and
// stores the label in the given label list if not already there.
// Returns the label's index in the label list.
//---------------------------------------------------------------------------

ULONG ReadLabel(FILE *fp, LabelList *labelList, ULONG *pLineNo)
{
   char token[TOKEN_LEN];
   char *endptr;
   Label label;

   ReadToken(token, fp, pLineNo);
   label.labelType = NUMERIC_LABEL;
   label.labelValue.numericLabel = strtod(token, &endptr);
   if (*endptr != '\0') 
   {
      label.labelType = STRING_LABEL;
      label.labelValue.stringLabel = token;
   }
   return StoreLabel(&label, labelList);
}


//---------------------------------------------------------------------------
// NAME:    ReadInteger
//
// INPUTS:  (FILE *fp) - file pointer from which number is read
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (ULONG) - integer read
//
// PURPOSE: Read an unsigned long integer from the given file.
//---------------------------------------------------------------------------

ULONG ReadInteger(FILE *fp, ULONG *pLineNo)
{
   ULONG i;
   char token[TOKEN_LEN];
   char *endptr;

   ReadToken(token, fp, pLineNo);
   i = strtoul(token, &endptr, 10);
   if (*endptr != '\0') 
   {
      fprintf(stderr, "Error: expecting integer in line %lu.\n",
              *pLineNo);
      exit(1);
   }
   return i;
}


//---------------------------------------------------------------------------
// NAME:    ReadToken
//
// INPUTS:  (char *token) - string into which token is copied
//          (FILE *fp) - file pointer from which token is read
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (int) - length of token read
//
// PURPOSE: Read the next token from the given file.  A token is
// defined as a string of non-whitespace characters, where whitespace
// includes spaces, tabs, newlines, comments, and EOF.
//---------------------------------------------------------------------------

int ReadToken(char *token, FILE *fp, ULONG *pLineNo)
{
   char ch;
   int i = 0;

   // skip whitespace and comments
   ch = fgetc(fp);
   while ((ch == SPACE) || (ch == TAB) || (ch == CARRIAGERETURN) ||
    (ch == NEWLINE) || (ch == COMMENT)) 
   {
      if (ch == NEWLINE)
         (*pLineNo)++;
      if (ch == COMMENT) 
      {
         while ((ch != NEWLINE) && (ch != (char)EOF))  // skip to end of line
            ch = fgetc(fp);
         if (ch == NEWLINE)
            (*pLineNo)++;
      }
      if (ch != (char)EOF)
         ch = fgetc(fp);
   }

   // read token
   if (ch == DOUBLEQUOTE) 
   { // read until reaching another double quote
      do 
      {
         token[i++] = ch;
         ch = fgetc(fp);
      } while ((ch != (char)EOF) && (ch != DOUBLEQUOTE));
      if (ch == DOUBLEQUOTE)
         token[i++] = ch;
      ch = fgetc(fp);
   } 
   else 
   { // read until reaching whitespace
      while ((ch != (char)EOF) && (ch != SPACE) && (ch != TAB) &&
             (ch != CARRIAGERETURN) && (ch != NEWLINE) && (ch != COMMENT)) 
      {
         token[i++] = ch;
         ch = fgetc(fp);
      }
   }
   token[i] = '\0';

   // if token ended by NEWLINE, increment lineNo
   if (ch == NEWLINE)
      (*pLineNo)++;

   // if token ended by comment, go ahead and skip comment
   if (ch == COMMENT) 
   {
      while ((ch != NEWLINE) && (ch != (char)EOF))
         ch = fgetc(fp);
      if (ch == NEWLINE)
         (*pLineNo)++;
   }

   return i;
}


//---------------------------------------------------------------------------
// NAME:    AllocateGraph
//
// INPUTS:  (ULONG v) - number of vertices in graph
//          (ULONG e) - number of edges in graph
//
// RETURN:  (Graph *) - pointer to newly-allocated graph
//
// PURPOSE: Allocate memory for new graph containing v vertices and e
// edges.
//---------------------------------------------------------------------------

Graph *AllocateGraph(ULONG v, ULONG e)
{
   Graph *graph;

   graph = (Graph *) malloc(sizeof(Graph));
   if (graph == NULL)
      OutOfMemoryError("AllocateGraph:graph");

   graph->numVertices = v;
   graph->numEdges = e;
   graph->vertices = NULL;
   graph->edges = NULL;
   if (v > 0) 
   {
      graph->vertices = (Vertex *) malloc(sizeof(Vertex) * v);
      if (graph->vertices == NULL)
         OutOfMemoryError("AllocateGraph:graph->vertices");
   }
   graph->vertexListSize = v;
   if (e > 0) 
   {
      graph->edges = (Edge *) malloc(sizeof(Edge) * e);
      if (graph->edges == NULL)
         OutOfMemoryError("AllocateGraph:graph->edges");
    }
   graph->edgeListSize = e;

   return graph;
}

//---------------------------------------------------------------------------
// NAME:    CopyGraph
//
// INPUTS:  (Graph *g) - graph to be copied
//
// RETURN:  (Graph *) - pointer to copy of graph
//
// PURPOSE: Create and return a copy of the given graph.
//---------------------------------------------------------------------------

Graph *CopyGraph(Graph *g)
{
   Graph *gCopy;
   ULONG nv;
   ULONG ne;
   ULONG v;
   ULONG e;
   ULONG numEdges;

   nv = g->numVertices;
   ne = g->numEdges;

   // allocate graph
   gCopy = AllocateGraph(nv, ne);

   // copy vertices; allocate and copy vertex edge arrays
   for (v = 0; v < nv; v++) 
   {
      gCopy->vertices[v].label = g->vertices[v].label;
      gCopy->vertices[v].map = g->vertices[v].map;
      gCopy->vertices[v].used = g->vertices[v].used;
      numEdges = g->vertices[v].numEdges;
      gCopy->vertices[v].numEdges = numEdges;
      gCopy->vertices[v].edges = NULL;
      if (numEdges > 0) 
      {
          gCopy->vertices[v].edges = (ULONG *) malloc(numEdges * sizeof(ULONG));
          if (gCopy->vertices[v].edges == NULL)
             OutOfMemoryError("CopyGraph:edges");
          for (e = 0; e < numEdges; e++)
             gCopy->vertices[v].edges[e] = g->vertices[v].edges[e];
      }
   }

   // copy edges
   for (e = 0; e < ne; e++) 
   {
      gCopy->edges[e].vertex1 = g->edges[e].vertex1;
      gCopy->edges[e].vertex2 = g->edges[e].vertex2;
      gCopy->edges[e].label = g->edges[e].label;
      gCopy->edges[e].directed = g->edges[e].directed;
      gCopy->edges[e].used = g->edges[e].used;
   }

   return gCopy;
}


//---------------------------------------------------------------------------
// NAME:    FreeGraph
//
// INPUTS:  (Graph *graph) - graph to be freed
//
// RETURN:  void
//
// PURPOSE: Free memory used by given graph, including the vertices array
// and the edges array for each vertex.
//---------------------------------------------------------------------------

void FreeGraph(Graph *graph)
{
   ULONG v;

   if (graph != NULL) 
   {
      for (v = 0; v < graph->numVertices; v++)
         free(graph->vertices[v].edges);
      free(graph->edges);
      free(graph->vertices);
      free(graph);
   }
}


//---------------------------------------------------------------------------
// NAME:    PrintGraph
//
// INPUTS:  (Graph *graph) - graph to be printed
//          (LabelList *labelList) - indexed list of vertex and edge labels
//
// RETURN:  void
//
// PURPOSE: Print the vertices and edges of the graph to stdout.
//---------------------------------------------------------------------------

void PrintGraph(Graph *graph, LabelList *labelList)
{
   ULONG v;
   ULONG e;

   if (graph != NULL) 
   {
      printf("  Graph(%luv,%lue):\n", graph->numVertices, graph->numEdges);
      // print vertices
      for (v = 0; v < graph->numVertices; v++) 
      {
         printf("    ");
         PrintVertex(graph, v, 0, labelList);
      }
      // print edges
      for (e = 0; e < graph->numEdges; e++) 
      {
         printf("    ");
         PrintEdge(graph, e, 0, labelList);
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintVertex
//
// INPUTS: (Graph *graph) - graph containing vertex
//         (ULONG vertexIndex) - index of vertex to print
//         (ULONG vertexOffset) - offset for vertex index in the
//           case that this is one of many examples
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print a vertex. Note that a graph containing many examples
// stores all vertices consecutively. Therefore, the first vertex of a
// later example will not have an index of 0, but an index of how many
// vertices came before in the number of examples. So, the vertexOffset
// is used to allow these vertices to be printed with indices corresponding
// to their order within the individual example, and not the entire graph
// of examples.
//---------------------------------------------------------------------------

void PrintVertex(Graph *graph, ULONG vertexIndex, ULONG vertexOffset, LabelList *labelList)
{
   printf("v %lu ", vertexIndex - vertexOffset + 1);
   PrintLabel(graph->vertices[vertexIndex].label, labelList);
   printf("\n");
}


//---------------------------------------------------------------------------
// NAME: PrintEdge
//
// INPUTS: (Graph *graph) - graph containing edge
//         (ULONG edgeIndex) - index of edge to print
//         (ULONG vertexOffset) - offset for vertex index in the
//           case that this is one of many examples
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print an edge. See note above on PrintVertex regarding the
// meaning of vertexOffset.
//---------------------------------------------------------------------------

void PrintEdge(Graph *graph, ULONG edgeIndex, ULONG vertexOffset, LabelList *labelList)
{
   Edge *edge = & graph->edges[edgeIndex];

   if (edge->directed)
      printf("d");
   else 
      printf("u");
   printf(" %lu %lu ", edge->vertex1 - vertexOffset + 1, edge->vertex2 - vertexOffset + 1);
   PrintLabel(edge->label, labelList);
   printf("\n");
}


//---------------------------------------------------------------------------
// NAME:    WriteGraphToFile
//
// INPUTS:  (FILE *outFile) - file stream to write graph
//          (Graph *graph) - graph to be written
//          (LabelList *labelList) - indexed list of vertex and edge labels
//          (ULONG vOffset) - vertex offset for compressed increments
//          (ULONG start) - beginning of vertex range to print
//          (ULONG finish) - end of vertex range to print
//          (BOOLEAN printPS) - flag indicating output is for predefined sub
//
// RETURN:  void
//
// PURPOSE: Write the vertices and edges of the graph to the given
//          file, prefaced by the SUB_TOKEN defined in subdue.h
//          (when WriteSubToken is TRUE).
//---------------------------------------------------------------------------

void WriteGraphToFile(FILE *outFile, Graph *graph, LabelList *labelList,
                      ULONG vOffset, ULONG start, ULONG finish, BOOLEAN printPS)
{
   ULONG v;
   ULONG e;
   Edge *edge = NULL;

   if (graph != NULL) 
   {
      if (printPS)
         fprintf(outFile, "%s\n", SUB_TOKEN);
      // write vertices
      for (v = start; v < finish; v++)
      {
         fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
         WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
      // write edges
      for (e = 0; e < graph->numEdges; e++)
      {
         edge = &graph->edges[e];
         if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
         {
            if (edge->directed)
               fprintf(outFile, "d");
            else
               fprintf(outFile, "u");
            fprintf(outFile, " %lu %lu ",
               (edge->vertex1 + 1 + vOffset - start),
               (edge->vertex2 + 1 + vOffset - start));
            WriteLabelToFile(outFile, edge->label, labelList, FALSE);
            fprintf(outFile, "\n");
         }
      }
      if (printPS)
         fprintf(outFile, "\n");
   }
}
