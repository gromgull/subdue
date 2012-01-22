//---------------------------------------------------------------------------
// incextend.c
//
// Functions for extending boundary instances with respect to a reference graph
//
// Much of this code was derived from the standard extend code
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME:  ExtendRefGraph
//
// INPUTS:  (ReferenceGraph *refGraph) - reference graph
//          (Substructure *bestSub) - the currently best substructure
//          (Graph *fullGraph) - complete graph
//          (Parameters *parameters) - system parameters
//
// RETURN:  (ReferenceGraph *) - updated reference graph
//
// PURPOSE:  Extend the ref graph by an edge or an edge and vertex in
//           all possible ways.
//---------------------------------------------------------------------------

ReferenceGraph *ExtendRefGraph(ReferenceGraph *refGraph, Substructure *bestSub,
                               Graph *fullGraph, Parameters *parameters)
{
   ReferenceGraph *newGraph;
   Vertex *vertex;
   Edge *edge;
   ULONG labelIndex;
   ULONG v2;
   ULONG secondVertex = 0;
   ULONG firstVertex;
   ULONG v;
   ULONG e;
   ULONG i;
   BOOLEAN found;
   BOOLEAN directed;

   newGraph = CopyReferenceGraph(refGraph);
   // When the new graph is created, it sizes the dynamic lists to the exact
   // number of vertices & edges.  The original graph may have bigger lists
   // because of the way the list sizes are incremented.
	
   MarkGraphEdgesUsed(refGraph, fullGraph, TRUE);
   // Mark the fullGraph edges invalid
   MarkGraphEdgesValid(refGraph, fullGraph, TRUE);
	
   for (v = 0; v < refGraph->numVertices; v++)
   {
      // If vertex has at least one valid edge connecting it to the graph
      if (refGraph->vertices[v].vertexValid)
      {
         vertex = &fullGraph->vertices[refGraph->vertices[v].map];
         for (e = 0; e < vertex->numEdges; e++)
         {
            edge = &fullGraph->edges[vertex->edges[e]];
            if (!edge->used && edge->validPath)
            {
               edge->used = TRUE;
               // get edge's other vertex
               if (edge->vertex1 == refGraph->vertices[v].map)
                  v2 = edge->vertex2;
               else 
                  v2 = edge->vertex1;
               // If second vertex is not in bestSub or if the second vertex is
               // already used in another full instance of bestSub, then do not
               // add the edge
               if (VertexInSub(bestSub->definition, &fullGraph->vertices[v2]) &&
                   !CheckVertexForOverlap(v2, bestSub, parameters))
               {
                  // check if edge's other vertex already in new reference graph
                  found = FALSE;
                  for (i = 0; ((i < newGraph->numVertices) && (! found)); i++)
        	  {
                     if (newGraph->vertices[i].map == v2)
        	     {
                        found = TRUE;
                        secondVertex = i;
                     }
                  }
                  if (!found)
        	  {
                     labelIndex = fullGraph->vertices[v2].label;
                     AddReferenceVertex(newGraph, labelIndex);
                     newGraph->vertices[newGraph->numVertices-1].map = v2;
                     secondVertex = newGraph->numVertices-1;
                  }
                  labelIndex = edge->label;
                  directed = edge->directed;

                  // Just to get the ordering right
                  if (v2 == edge->vertex2)
                     firstVertex = v;
                  else
        	  {
                     firstVertex = secondVertex;
                     secondVertex = v;
                  }
                  AddReferenceEdge(newGraph, firstVertex, secondVertex,
                                   directed, labelIndex, FALSE);
                  newGraph->edges[newGraph->numEdges-1].map = vertex->edges[e];
               }
            }
         }
      }
   }

   MarkGraphEdgesUsed(newGraph, fullGraph, FALSE);
   //reset the fullGraph edges
   MarkGraphEdgesValid(refGraph, fullGraph, FALSE);

   return newGraph;
}


//-----------------------------------------------------------------------------
// NAME:  ExtendConstrainedInstance
//
// INPUTS:  (Instance *instance) - current instance
//          (Substructure *bestSub) - currently best substructure
//          (ReferenceGraph *refGraph) - reference graph
//          (Graph *fullGraph) - complete graph
//          (Parameters *parameters) - system parameters
//
// RETURN:  (InstanceList *) - list of extended instances
//
// PURPOSE:  Extend an instance in all possible ways.
//-----------------------------------------------------------------------------

InstanceList *ExtendConstrainedInstance(Instance *instance,
                                        Substructure *bestSub, 
                                        ReferenceGraph *refGraph, 
                                        Graph *fullGraph,
                                        Parameters *parameters)
{
   InstanceList *newInstanceList;
   Instance *newInstance;
   ULONG v;
   ULONG e;
   ULONG v2;
   ReferenceVertex *vertex;
   ReferenceVertex *vertex2;
   ReferenceEdge *edge;
   BOOLEAN listEmpty = TRUE;

   newInstanceList = NULL;
   MarkRefGraphInstanceEdges(instance, refGraph, TRUE);
   for (v = 0; v < instance->numVertices; v++)
   {
      vertex = &refGraph->vertices[instance->vertices[v]];
      if (vertex->vertexValid)
      {                // I do not think we can ever have an invalid vertex here
         for (e = 0; e < vertex->numEdges; e++)
         {
            edge = &refGraph->edges[vertex->edges[e]];
            if (!edge->used && !edge->failed)
            {
               // add new instance to list
               if (edge->vertex1 == instance->vertices[v])
                  v2 = edge->vertex2;
               else 
                  v2 = edge->vertex1;
               vertex2 = &refGraph->vertices[v2];
               // Now check to see if this vertex is a candidate for expansion
               if (VertexInSub(bestSub->definition,
                               &fullGraph->vertices[vertex2->map]) &&
                   !CheckVertexForOverlap(vertex2->map, bestSub, parameters))
               {
                  newInstance =
                     CreateConstrainedExtendedInstance(instance,
                        instance->vertices[v], v2, vertex->edges[e], refGraph);
                  if (listEmpty) 
                  {
                     newInstanceList = AllocateInstanceList();
                     listEmpty = FALSE;
                  }
                  InstanceListInsert(newInstance, newInstanceList, TRUE);
               }
            }
         }
      }
   }
   MarkRefGraphInstanceEdges(instance, refGraph, FALSE);

   return newInstanceList;
}


//--------------------------------------------------------------------------
// NAME:  CreateConstrainedExtendedInstance
//
// INPUTS: (Instance *instance) - current instance
//         (ULONG v) - vertex being extended from
//         (ULONG v2) - vertex being extended to
//         (ULONG e) - edge being extended
//         (ReferenceGraph *graph)
//
// RETURN: (Instance *) - extended instance
//
// PURPOSE:  Create one new extended instance.
//--------------------------------------------------------------------------

Instance *CreateConstrainedExtendedInstance(Instance *instance, ULONG v,
                                            ULONG v2, ULONG e, 
                                            ReferenceGraph *graph)
{
   Instance *newInstance;
   BOOLEAN found = FALSE;
   ULONG i;

   // check if edge's other vertex is already in instance
   for (i = 0; ((i < instance->numVertices) && (! found)); i++)
   {
      if (instance->vertices[i] == v2)
         found = TRUE;
   }
	
   if (!found)
      newInstance = AllocateInstance(instance->numVertices + 1,
                                     instance->numEdges + 1);
   else 
      newInstance = AllocateInstance(instance->numVertices,
                                     instance->numEdges + 1);

   // Set vertices of new instance, kept in increasing order
   for (i = 0; i < instance->numVertices; i++)
      newInstance->vertices[i] = instance->vertices[i];
   newInstance->newVertex = VERTEX_UNMAPPED;
   if (!found)
   {
      i = instance->numVertices;
      while ((i > 0) && (v2 < newInstance->vertices[i-1]))
      {
         newInstance->vertices[i] = newInstance->vertices[i-1];
         i--;
      }
      newInstance->vertices[i] = v2;
      newInstance->newVertex = i;
   }

   // set edges of new instance, kept in increasing order
   for (i = 0; i < instance->numEdges; i++)
      newInstance->edges[i] = instance->edges[i];
   i = instance->numEdges;
   while ((i > 0) && (e < newInstance->edges[i-1]))
   {
      newInstance->edges[i] = newInstance->edges[i-1];
      i--;
   }
   newInstance->edges[i] = e;
   newInstance->newEdge = i;

   return newInstance;
}
