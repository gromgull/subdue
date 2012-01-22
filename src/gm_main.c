//---------------------------------------------------------------------------
// gm_main.c
//
// Main function for standalone graph matcher.
//
// Usage: gm g1 g2
//
// The inexact graph match program computes the cost of transforming
// the larger of the input graphs into the smaller according to the
// transformation costs defined in subdue.h.  The program returns this
// cost and the mapping of vertices in the larger graph to vertices in
// the smaller graph.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


void PrintMapping(VertexMap *, ULONG);


//---------------------------------------------------------------------------
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main function for standalone graph matcher.  Takes two
// command-line arguments, which are the graph file names containing
// the graphs to be matched.
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
   Graph *g1, *g2;
   LabelList *labelList = NULL;
   BOOLEAN directed = TRUE; // 'e' edges are considered directed
   ULONG maxVertices;
   VertexMap *mapping;
   double matchCost;

   if (argc != 3) 
   {
      fprintf(stderr, "usage: %s <graph file> <graph file>\n", argv[0]);
      exit(1);
   }
   labelList = AllocateLabelList();
   g1 = ReadGraph(argv[1], labelList, directed);
   g2 = ReadGraph(argv[2], labelList, directed);

   if (g1->numVertices < g2->numVertices) 
   {
      maxVertices = g2->numVertices;
      mapping = (VertexMap *) malloc(sizeof(VertexMap) * maxVertices);
      matchCost = InexactGraphMatch(g2, g1, labelList, MAX_DOUBLE, mapping);
   } 
   else 
   {
      maxVertices = g1->numVertices;
      mapping = (VertexMap *) malloc(sizeof(VertexMap) * maxVertices);
      matchCost = InexactGraphMatch(g1, g2, labelList, MAX_DOUBLE, mapping);
   }

   printf("Match Cost = %f\n", matchCost);
   PrintMapping(mapping, maxVertices);

   free(mapping);

   return 0;
}


//---------------------------------------------------------------------------
// NAME: PrintMapping
//
// INPUTS: (VertexMap *) - mapping
//         (ULONG numVertices) - number of vertices mapped in mapping
//
// RETURN: (void)
//
// PURPOSE: Print given vertex mapping in order by first vertex.
//---------------------------------------------------------------------------

void PrintMapping(VertexMap *mapping, ULONG numVertices)
{
   ULONG *sortedMapping;
   ULONG i;

   sortedMapping = (ULONG *) malloc(sizeof(ULONG) * numVertices);
   if (sortedMapping == NULL)
      OutOfMemoryError("sortedMapping");

   for (i = 0; i < numVertices; i++)
      sortedMapping[mapping[i].v1] = mapping[i].v2;

   printf("Mapping (vertices of larger graph to smaller):\n");
   for (i = 0; i < numVertices; i++) 
   {
      printf("  %lu -> ", i+1);
      if (sortedMapping[i] == VERTEX_DELETED)
         printf("deleted\n");
      else if (sortedMapping[i] == VERTEX_UNMAPPED) // this should never happen
         printf("unmapped\n");
      else 
         printf("%lu\n", sortedMapping[i] + 1);
   }

   free(sortedMapping);
}

