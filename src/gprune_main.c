//---------------------------------------------------------------------------
// gprune_main.c
//
// Main function for graph pruner, which removes vertices and edges with
// specific labels.
//
// Usage: gprune <label> <inputgraphfile> <outputgraphfile>
//
// Removes all vertices and edges with label equal to <label> in the input
// graph file and outputs the result to the output graph file.  When
// removing a vertex, all its edges are removed.  When removing an edge
// (either because it has <label> or was removed as the result of a vertex
// removal), any vertices left without a connecting edge are also removed.
//
// If the input file contains several examples, then each is pruned
// separately.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"

#define TMP_FILE_NAME "/tmp/gprune.tmp.g"


// Prototypes

void PruneGraph(Graph *, char *, LabelList *);
void WritePrunedGraph(Graph *, FILE *, LabelList *);


//---------------------------------------------------------------------------
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main function for graph pruner.  Takes three command-line
// arguments, which are the label to remove, the input graph file to remove
// from, and the output graph file to write to.  If graph contains multiple
// XP/XN examples, then this file is split, pruned and then reconstituted.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   FILE *inFile;
   FILE *outFile;
   FILE *tmpFile;
   char buffer[TOKEN_LEN];
   char XString[TOKEN_LEN];
   char *result;
   Graph *g;
   LabelList *labelList;
   BOOLEAN directed = TRUE; // 'e' edges are considered directed

   if (argc != 4) 
   {
      fprintf(stderr,
              "usage: %s <label> <input graph file> <output graph file>\n",
              argv[0]);
      exit(1);
   }

   inFile = fopen(argv[2], "r");
   if (inFile == NULL) 
   {
      fprintf(stderr, "unable to open input file: %s\n", argv[2]);
      exit(1);
   }

   outFile = fopen(argv[3], "w");
   if (outFile == NULL) 
   {
      fprintf(stderr, "unable to open output file: %s\n", argv[3]);
      exit(1);
   }

   result = fgets(buffer, TOKEN_LEN, inFile);
   while (result != NULL) 
   {
      strcpy(XString, "");
      if (buffer[0] == 'X') 
      {
         strcpy(XString, buffer);
         result = fgets(buffer, TOKEN_LEN, inFile);
      }
      // write temporary graph file
      tmpFile = fopen(TMP_FILE_NAME, "w");
      if (tmpFile == NULL) 
      {
         fprintf(stderr, "unable to open temp file: %s\n", TMP_FILE_NAME);
         exit(1);
      }
      while ((result != NULL) && (buffer[0] != 'X')) 
      {
         fprintf(tmpFile, "%s", buffer);
         result = fgets(buffer, TOKEN_LEN, inFile);
      }
      fclose(tmpFile);
      // prune graph
      labelList = AllocateLabelList();
      g = ReadGraph(TMP_FILE_NAME, labelList, directed);
      PruneGraph(g, argv[1], labelList);
      fprintf(outFile, "%s", XString);
      WritePrunedGraph(g, outFile, labelList);
      FreeGraph(g);
      FreeLabelList(labelList);
   }

   fclose(outFile);
   fclose(inFile);
   return 0;
}


//---------------------------------------------------------------------------
// NAME: PruneGraph
//
// INPUTS: (Graph *g) - graph to be pruned
//         (char *labelStr) - label to remove from vertices and edges
//         (LabelList *labelList) - list of labels in graph
//
// RETURN: (void)
//
// PURPOSE: Remove all vertices and edges in graph g having given label,
// along with any vertices left hanging.
//---------------------------------------------------------------------------

void PruneGraph(Graph *g, char *labelStr, LabelList *labelList)
{
   ULONG v, e;
   char *endptr;
   Label label;
   ULONG labelIndex;
   BOOLEAN hangingVertex;
   ULONG vertexCounter;

   // initialize graph
   for (v = 0; v < g->numVertices; v++) 
   {
      g->vertices[v].used = TRUE;
      g->vertices[v].map = v;
   }
   for (e = 0; e < g->numEdges; e++)
      g->edges[e].used = TRUE;

   // create label structure and lookup index
   label.labelType = NUMERIC_LABEL;
   label.labelValue.numericLabel = strtod(labelStr, &endptr);
   if (*endptr != '\0') 
   {
      label.labelType = STRING_LABEL;
      label.labelValue.stringLabel = labelStr;
   }
   labelIndex = GetLabelIndex(&label, labelList);

   // remove any vertices with label
   for (v = 0; v < g->numVertices; v++) 
   {
      if (g->vertices[v].label == labelIndex) 
      {
         // delete vertex
         g->vertices[v].used = FALSE;
         // and delete all its edges
         for (e = 0; e < g->vertices[v].numEdges; e++)
            g->edges[g->vertices[v].edges[e]].used = FALSE;
      }
   }

   // remove any edges with label
   for (e = 0; e < g->numEdges; e++)
      if (g->edges[e].label == labelIndex)
         g->edges[e].used = FALSE;

   // remove any vertices with no connecting edges
   for (v = 0; v < g->numVertices; v++) 
   {
      hangingVertex = TRUE;
      for (e = 0; e < g->vertices[v].numEdges; e++)
         if (g->edges[g->vertices[v].edges[e]].used == TRUE)
      hangingVertex = FALSE;
      if (hangingVertex == TRUE)
         g->vertices[v].used = FALSE;
   }

   // renumber vertices via their map field
   vertexCounter = 0;
   for (v = 0; v < g->numVertices; v++)
      if (g->vertices[v].used == TRUE) 
      {
         g->vertices[v].map = vertexCounter;
         vertexCounter++;
      }
}


//---------------------------------------------------------------------------
// NAME: WritePrunedGraph
//
// INPUTS: (Graph *g) - pruned graph
//         (FILE *outFile) - output file
//         (LabelList *labelList) - list of labels in graph
//
// RETURN: (void)
//
// PURPOSE: Writes the graph g to the given output file.  WritePrunedGraph
// assumes that PruneGraph has just been called inorder to properly set the
// vertex used and map fields and the edge used fields.
//---------------------------------------------------------------------------

void WritePrunedGraph(Graph *g, FILE *outFile, LabelList *labelList)
{
   ULONG v, e;
   Edge *edge;

   // write vertices
   for (v = 0; v < g->numVertices; v++)
      if (g->vertices[v].used == TRUE) 
      {
         fprintf(outFile, "v %lu ", (g->vertices[v].map + 1));
         WriteLabelToFile(outFile, g->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
   // write edges
   for (e = 0; e < g->numEdges; e++)
      if (g->edges[e].used == TRUE) 
      {
         edge = & g->edges[e];
         if (edge->directed)
            fprintf(outFile, "d");
         else 
            fprintf(outFile, "u");
         fprintf(outFile, " %lu %lu ",
                 (g->vertices[edge->vertex1].map + 1),
                 (g->vertices[edge->vertex2].map + 1));
         WriteLabelToFile(outFile, edge->label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
}
