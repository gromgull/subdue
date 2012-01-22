//---------------------------------------------------------------------------
// dot.c
//
// Functions for writing Subdue graphs in dot format.  The dot format
// is part of AT&T Labs GraphViz project.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: WriteGraphToDotFile
//
// INPUTS: (char *dotFileName) - file to write dot format
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Writes the positive and negative (if exists) graphs from
// parameters to dotFileName in dot format.  The positive graph is written
// in black, and the negative graph is written in red.
//---------------------------------------------------------------------------

void WriteGraphToDotFile(char *dotFileName, Parameters *parameters)
{
   FILE *dotFile;
   ULONG v;
   ULONG e;
   Graph *posGraph;
   Graph *negGraph;
   ULONG vertexOffset;

   // parameters used
   LabelList *labelList = parameters->labelList;
   posGraph = parameters->posGraph;
   negGraph = parameters->negGraph;

   // open dot file for writing
   dotFile = fopen(dotFileName, "w");
   if (dotFile == NULL) 
   {
      printf("ERROR: unable to write to dot output file %s\n", dotFileName);
      exit (1);
   }

   // write beginning of dot file
   fprintf(dotFile, "// Subdue %s graph in dot format\n\n", SUBDUE_VERSION);
   fprintf(dotFile, "digraph SubdueGraph {\n");

   // write positive graph to dot file
   vertexOffset = 0;
   for (v = 0; v < posGraph->numVertices; v++)
      WriteVertexToDotFile(dotFile, v, vertexOffset, posGraph, labelList,
                           "black");
   for (e = 0; e < posGraph->numEdges; e++)
      WriteEdgeToDotFile(dotFile, e, vertexOffset, posGraph, labelList,
                         "black");

   // write negative graph to dot file
   if (negGraph != NULL) 
   {
      vertexOffset = posGraph->numVertices;
      for (v = 0; v < negGraph->numVertices; v++)
         WriteVertexToDotFile(dotFile, v, vertexOffset, negGraph, labelList,
                              "red");
      for (e = 0; e < negGraph->numEdges; e++)
         WriteEdgeToDotFile(dotFile, e, vertexOffset, negGraph, labelList,
                            "red");
   }

   // write end of dot file
   fprintf(dotFile, "}\n");

   fclose(dotFile);
}


//---------------------------------------------------------------------------
// NAME: WriteGraphWithInstancesToDotFile
//
// INPUTS: (char *dotFileName) - file to write dot format
//         (Graph *graph) - graph to write
//         (InstanceList *instanceList) - instances to highlight in graph
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Write graph to dotFileName in dot format, highlighting
// instances in blue.
//---------------------------------------------------------------------------

void WriteGraphWithInstancesToDotFile(char *dotFileName, Graph *graph,
                                      InstanceList *instanceList,
                                      Parameters *parameters)
{
   FILE *dotFile;
   InstanceListNode *instanceListNode;
   Instance *instance;
   ULONG i;
   ULONG v;
   ULONG e;
   ULONG vertexOffset;

   // parameters used
   LabelList *labelList = parameters->labelList;

   // open dot file for writing
   dotFile = fopen(dotFileName, "w");
   if (dotFile == NULL) 
   {
      printf("ERROR: unable to write to dot output file %s\n", dotFileName);
      exit (1);
   }

   // write beginning of dot file
   fprintf(dotFile, "// Subdue %s graph in dot format\n\n", SUBDUE_VERSION);
   fprintf(dotFile, "digraph SubdueGraph {\n");

   vertexOffset = 0; // always zero for writing just one graph
   // first write instances of graph to dot file
   i = 0;
   instanceListNode = instanceList->head;
   while (instanceListNode != NULL) 
   {
      instance = instanceListNode->instance;
      for (v = 0; v < instance->numVertices; v++)
         WriteVertexToDotFile(dotFile, instance->vertices[v], vertexOffset,
                              graph, labelList, "blue");
      for (e = 0; e < instance->numEdges; e++)
         WriteEdgeToDotFile(dotFile, instance->edges[e], vertexOffset,
                            graph, labelList, "blue");
      MarkInstanceVertices(instance, graph, TRUE);
      MarkInstanceEdges(instance, graph, TRUE);
      instanceListNode = instanceListNode->next;
      i++;
   }

   // write rest of graph to dot file
   for (v = 0; v < graph->numVertices; v++)
      if (! graph->vertices[v].used)
         WriteVertexToDotFile(dotFile, v, vertexOffset, graph, labelList,
                              "black");
   for (e = 0; e < graph->numEdges; e++)
      if (! graph->edges[e].used)
         WriteEdgeToDotFile(dotFile, e, vertexOffset, graph, labelList,
                            "black");

   // write end of dot file
   fprintf(dotFile, "}\n");
   fclose(dotFile);

   // unmark instance vertices and edges
   instanceListNode = instanceList->head;
   while (instanceListNode != NULL) 
   {
      instance = instanceListNode->instance;
      MarkInstanceVertices(instance, graph, FALSE);
      MarkInstanceEdges(instance, graph, FALSE);
      instanceListNode = instanceListNode->next;
   }
}


//---------------------------------------------------------------------------
// NAME: WriteSubsToDotFile
//
// INPUTS: (char *dotFileName) - file to write dot format
//         (Graph **subGraphs) - substructure graphs to write
//         (ULONG numSubGraphs) - number of substructures to write
//
// RETURN: (void)
//
// PURPOSE: Write given substructure graphs in dot format to
// dotFileName.  Substructures are defined as dot subgraphs, and their
// IDs must begin with "cluster" in order to support edges between
// substructures.
//---------------------------------------------------------------------------

void WriteSubsToDotFile(char *dotFileName, Graph **subGraphs,
                        ULONG numSubGraphs, Parameters *parameters)
{
   FILE *dotFile;
   Graph *graph;
   ULONG *subVertexIndices;
   ULONG vertexOffset;
   ULONG subNumber;
   ULONG i;
   ULONG v;
   ULONG e;

   // parameters used
   LabelList *labelList = parameters->labelList;

   // open dot file for writing
   dotFile = fopen(dotFileName, "w");
   if (dotFile == NULL) 
   {
      printf("ERROR: unable to write to dot output file %s\n", dotFileName);
      exit(1);
   }

   // allocate array to hold last vertex number of each sub-graph
   subVertexIndices = (ULONG *) malloc(sizeof(ULONG) * numSubGraphs);
   if (subVertexIndices == NULL)
      OutOfMemoryError("WriteSubsToDotFile:subVertexIndices");

   // write beginning of dot file
   fprintf(dotFile, "// Subdue %s output in dot format\n\n", SUBDUE_VERSION);
   fprintf(dotFile, "digraph Subdue {\n\n");
   fprintf(dotFile, "  compound=true;\n\n"); // allows edges between subs

   vertexOffset = 0;
   // write each sub-graph to dot file
   for (i = 0; i < numSubGraphs; i++) {

      fprintf(dotFile, "subgraph cluster_%s_%lu {\n",
              SUB_LABEL_STRING, (i + 1));
      graph = subGraphs[i];
      for (v = 0; v < graph->numVertices; v++)
         WriteVertexToDotFile(dotFile, v, vertexOffset, graph, labelList,
                              "black");
      for (e = 0; e < graph->numEdges; e++)
         WriteEdgeToDotFile(dotFile, e, vertexOffset, graph, labelList,
                            "black");
      fprintf(dotFile, "  label=\"%s_%lu\";\n}\n",
              SUB_LABEL_STRING, (i + 1));

      // add edges from other sub-graphs, if any
      for (v = 0; v < graph->numVertices; v++) 
      {
         subNumber = SubLabelNumber(graph->vertices[v].label, labelList);
         if (subNumber > 0) 
         { // then valid reference to previous sub
            // write edge between substructure cluster sub-graphs
            fprintf(dotFile, "%lu -> %lu ",
                    subVertexIndices[subNumber - 1], (vertexOffset + 1));
            fprintf(dotFile, "[ltail=cluster_%s_%lu,lhead=cluster_%s_%lu];\n",
                    SUB_LABEL_STRING, subNumber, SUB_LABEL_STRING, (i + 1));
         }
      }
      fprintf(dotFile, "\n");
      vertexOffset += graph->numVertices;
      subVertexIndices[i] = vertexOffset; // set index to last vertex of sub
   }

   // write end of dot file
   fprintf(dotFile, "}\n");

   fclose(dotFile);
}


//---------------------------------------------------------------------------
// NAME: WriteVertexToDotFile
//
// INPUTS: (FILE *dotFile) - file stream for writing dot output
//         (ULONG v) - vertex index in graph to write
//         (ULONG vertexOffset) - offset for v's printed vertex number
//         (Graph *graph) - graph containing vertex
//         (LabelList *labelList) - graph's labels
//         (char *color) - string indicating color for vertex
//
// RETURN: (void)
//
// PURPOSE: Writes the vertex to the dot file.
//---------------------------------------------------------------------------

void WriteVertexToDotFile(FILE *dotFile, ULONG v, ULONG vertexOffset,
                          Graph *graph, LabelList *labelList, char *color)
{
   fprintf(dotFile, "  %lu [label=\"", (v + vertexOffset + 1));
   WriteLabelToFile(dotFile, graph->vertices[v].label, labelList, TRUE);
   fprintf(dotFile, "\",color=%s,fontcolor=%s];\n", color, color);
}


//---------------------------------------------------------------------------
// NAME: WriteEdgeToDotFile
//
// INPUTS: (FILE *dotFile) - file stream for writing dot output
//         (ULONG e) - edge index in graph to write
//         (ULONG vertexOffset) - offset for vertex's printed numbers
//         (Graph *graph) - graph containing edge
//         (LabelList *labelList) - graph's labels
//         (char *color) - string indicating color for vertex
//
// RETURN: (void)
//
// PURPOSE: Writes the edge to the dot file.
//---------------------------------------------------------------------------

void WriteEdgeToDotFile(FILE *dotFile, ULONG e, ULONG vertexOffset,
                        Graph *graph, LabelList *labelList, char *color)
{
   Edge *edge;

   edge = & graph->edges[e];
   fprintf(dotFile, "  %lu -> %lu [label=\"",
           (edge->vertex1 + vertexOffset + 1),
           (edge->vertex2 + vertexOffset + 1));
   WriteLabelToFile(dotFile, edge->label, labelList, TRUE);
   fprintf(dotFile, "\"");
   if (! edge->directed)
      fprintf(dotFile, ",arrowhead=none");
   fprintf(dotFile, ",color=%s,fontcolor=%s];\n", color, color);
}
