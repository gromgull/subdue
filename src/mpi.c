//---------------------------------------------------------------------------
// mpi.c
//
// Functions for parallel version of Subdue using MPI.
//
// Subdue 5
//---------------------------------------------------------------------------

#include <mpi.h>
#include "subdue.h"


//----- Master Process Functions

//---------------------------------------------------------------------------
// NAME: ReceiveBestSubFromChild
//
// INPUTS: (int *childID) - child process received from (returned by
//                          reference)
//         (Parameters *parameters)
//
// RETURN: (Substructure *) - pointer to received substructure
//
// PURPOSE: Receives the best susbtructure sent by any child.  The
// substructure is allocated, and a pointer to it is returned.  The child
// process number is also returned by reference through childID.
//---------------------------------------------------------------------------

Substructure *ReceiveBestSubFromChild(int *childID, Parameters *parameters)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;
   MPI_Status status;
   Substructure *sub;

   MPI_Recv(buffer, MPI_BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE,
            MPI_MSG_BEST_SUB, MPI_COMM_WORLD, &status);
   position = 0;
   sub = UnpackSubstructure(buffer, &position, parameters);
   *childID = status.MPI_SOURCE;
   return sub;
}


//---------------------------------------------------------------------------
// NAME: UniqueChildSub
//
// INPUTS: (Substructure *sub) - sub to look for in array of subs
//         (Substructure **subs) - array of substructures
//         (Parameters *parameters)
//
// RETURN: TRUE is sub not found in array of subs; else FALSE
//
// PURPOSE: Checks to see if there is already a substructure in the subs
// array that matches the given sub.
//---------------------------------------------------------------------------

BOOLEAN UniqueChildSub(Substructure *sub, Substructure **subs,
                       Parameters *parameters)
{
   ULONG i;

   for (i = 1; i <= parameters->numPartitions; i++) 
   {
      if (subs[i] != NULL)
         if (GraphMatch(sub->definition, subs[i]->definition,
                        parameters->labelList, 0.0, NULL, NULL))
            return FALSE;
   }
   return TRUE;
}


//---------------------------------------------------------------------------
// NAME: SendEvalSubToChild
//
// INPUTS: (Substructure *sub) - substructure for child to evaluate
//         (Parameters *parameters)
//         (int childID) - child process number to evaluate sub
//
// RETURN: (void)
//
// PURPOSE: Sends the given substructure to the indicated childID for
// evaluation.  Note that sub may be NULL.
//---------------------------------------------------------------------------

void SendEvalSubToChild(Substructure *sub, Parameters *parameters,
                        int childID)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;

   position = 0;
   PackSubstructure(sub, parameters, buffer, &position);
   MPI_Send(buffer, position, MPI_PACKED, childID, MPI_MSG_EVAL_SUB,
            MPI_COMM_WORLD);
}


//---------------------------------------------------------------------------
// NAME: ReceiveEvalFromChild
//
// INPUTS: (double *subValue) - returns received substructure value
//         (ULONG *numInstances) - returns number of instances
//         (ULONG *numNegInstances) - returns number of instances in
//           negative graph
//
// RETURN: (void)
//
// PURPOSE: Returns by reference the value, positive graph instances, and
// negative graph instances received from child.
//---------------------------------------------------------------------------

void ReceiveEvalFromChild(double *subValue, ULONG *numInstances,
                          ULONG *numNegInstances)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;
   MPI_Status status;

   MPI_Recv(buffer, MPI_BUFFER_SIZE, MPI_PACKED, MPI_ANY_SOURCE,
            MPI_MSG_EVAL, MPI_COMM_WORLD, &status); 

   position = 0;
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, &position, subValue, 1, MPI_DOUBLE,
              MPI_COMM_WORLD);
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, &position, numInstances, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, &position, numNegInstances, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
}


//---------------------------------------------------------------------------
// NAME: SendBestSubToChildren
//
// INPUTS: (Substructure *sub) - substructure to send to all child processes
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Send given substructure to all child processes as the best
// global substructure.
//---------------------------------------------------------------------------

void SendBestSubToChildren(Substructure *sub, Parameters *parameters)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;
   int childID;

   position = 0;
   PackSubstructure(sub, parameters, buffer, &position);
   for (childID = 1; childID <= parameters->numPartitions; childID++)
      MPI_Send(buffer, position, MPI_PACKED, childID, MPI_MSG_BEST_SUB,
               MPI_COMM_WORLD);
}


//----- Child Process Functions

//---------------------------------------------------------------------------
// NAME: SendBestSubToMaster
//
// INPUTS: (Substructure *sub) - substructure to send to master process
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Sends child's best substructure to the master process.  The
// message consists of the substructure's value, number of positive and
// negative instances, and definition.
//---------------------------------------------------------------------------

void SendBestSubToMaster(Substructure *sub, Parameters *parameters)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;

   position = 0;
   PackSubstructure(sub, parameters, buffer, &position);
   MPI_Send(buffer, position, MPI_PACKED, 0, MPI_MSG_BEST_SUB,
            MPI_COMM_WORLD);
}


//---------------------------------------------------------------------------
// NAME: ReceiveEvalSubFromMaster
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (Substructure *) - pointer to received substructure
//
// PURPOSE: Receives a susbtructure sent by the master process for
// evaluation.  The substructure is allocated, and a pointer to it is
// returned.
//---------------------------------------------------------------------------

Substructure *ReceiveEvalSubFromMaster(Parameters *parameters)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;
   MPI_Status status;
   Substructure *sub;

   MPI_Recv(buffer, MPI_BUFFER_SIZE, MPI_PACKED, 0, MPI_MSG_EVAL_SUB,
            MPI_COMM_WORLD, &status);
   position = 0;
   sub = UnpackSubstructure(buffer, &position, parameters);
   return sub;
}


//---------------------------------------------------------------------------
// NAME: SendEvalToMaster
//
// INPUTS: (double subValue) - substructure's value
//         (ULONG numInstances) - number of positive instances
//         (ULONG numNegInstances) - number of negative instances
//
// RETURN: (void)
//
// PURPOSE: Send substructure's value, positive instances, and negative
// instances to the master process as a packed message.
//---------------------------------------------------------------------------

void SendEvalToMaster(double subValue, ULONG numInstances,
             ULONG numNegInstances)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;

   position = 0;
   MPI_Pack(&subValue, 1, MPI_DOUBLE, buffer, MPI_BUFFER_SIZE,
            &position, MPI_COMM_WORLD);
   MPI_Pack(&numInstances, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            &position, MPI_COMM_WORLD);
   MPI_Pack(&numNegInstances, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            &position, MPI_COMM_WORLD);

   MPI_Send(buffer, position, MPI_PACKED, 0, MPI_MSG_EVAL, MPI_COMM_WORLD);
}


//---------------------------------------------------------------------------
// NAME: ReceiveBestSubFromMaster
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (Substructure *) - pointer to received substructure
//
// PURPOSE: Receives the best susbtructure sent by the master process.  The
// substructure is allocated, and a pointer to it is returned.
//---------------------------------------------------------------------------

Substructure *ReceiveBestSubFromMaster(Parameters *parameters)
{
   char buffer[MPI_BUFFER_SIZE];
   int position;
   MPI_Status status;
   Substructure *sub;

   MPI_Recv(buffer, MPI_BUFFER_SIZE, MPI_PACKED, 0, MPI_MSG_BEST_SUB,
            MPI_COMM_WORLD, &status);
   position = 0;
   sub = UnpackSubstructure(buffer, &position, parameters);
   return sub;
}


//---------------------------------------------------------------------------
// NAME: FindEvalSub
//
// INPUTS: (Substructure *sub) - substructure to find in sub list
//         (SubList *evalSubs) - list to look for sub in
//         (Parameters *parameters)
//
// RETURN: Pointer to substructure in evalSubs list matching sub.
//
// PURPOSE: Finds the substructure in evalSubs that matches the given sub.
// Returns a pointer to the substructure.  Matching substructure should
// exist and be unique.
//---------------------------------------------------------------------------

Substructure *FindEvalSub(Substructure *sub, SubList *evalSubs,
                          Parameters *parameters)
{
   SubListNode *subListNode;

   if (evalSubs != NULL) 
   {
      subListNode = evalSubs->head;
      while (subListNode != NULL) 
      {
         if (GraphMatch(sub->definition, subListNode->sub->definition,
                        parameters->labelList, 0.0, NULL, NULL))
            return subListNode->sub;
         subListNode = subListNode->next;
      }
   }
   return NULL;
}


//----- General Functions

//---------------------------------------------------------------------------
// NAME: PackSubstructure
//
// INPUTS: (Substructure *sub) - substructure to pack into buffer
//         (Parameters *parameters)
//         (char *buffer) - buffer to hold packed substructure
//         (int *position) - position of end of packed buffer (by reference)
//
// RETURN: (void)
//
// PURPOSE: Packs the given substructure into an MPI message buffer.  The
// reference to position is set to the end of the buffer.  Note that sub
// may be NULL.
//---------------------------------------------------------------------------

void PackSubstructure(Substructure *sub, Parameters *parameters,
             char *buffer, int *position)
{
   Graph *graph;
   double value;
   ULONG numInstances;
   ULONG numNegInstances;

   // set parameters of substructure
   value = 0.0;
   numInstances = 0;
   numNegInstances = 0;
   graph = NULL;
   if (sub != NULL) 
   {
      value = sub->value;
      numInstances = sub->numInstances;
      numNegInstances = sub->numNegInstances;
      graph = sub->definition;
   }

   // pack parameters of substructure
   MPI_Pack(&value, 1, MPI_DOUBLE, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);
   MPI_Pack(&numInstances, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);
   MPI_Pack(&numNegInstances, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);
 
   PackGraph(graph, parameters, buffer, position);
}


//---------------------------------------------------------------------------
// NAME: PackGraph
//
// INPUTS: (Graph *graph) - graph to pack into MPI message buffer
//         (Parameters *parameters)
//         (char *buffer) - MPI message buffer
//         (int *position) - position of end of buffer (by reference)
//
// RETURN: (void)
//
// PURPOSE: Pack the information in the given graph into the MPI message
// buffer.  Note that graph may by NULL.
//---------------------------------------------------------------------------

void PackGraph(Graph *graph, Parameters *parameters, char *buffer,
               int *position)
{
   ULONG numVertices;
   ULONG numEdges;
   ULONG v, e;
   Edge *edge;

   numVertices = 0;
   numEdges = 0;
   if (graph != NULL) 
   {
      numVertices = graph->numVertices;
      numEdges = graph->numEdges;
   }
   MPI_Pack(&numVertices, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);
   MPI_Pack(&numEdges, 1, MPI_UNSIGNED_LONG, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);

   // pack vertices and edges of graph
   for (v = 0; v < numVertices; v++) 
   {
      PackLabel(graph->vertices[v].label, parameters->labelList, buffer,
                position);
   }
   for (e = 0; e < numEdges; e++) 
   {
      edge = & graph->edges[e];
      MPI_Pack(&(edge->directed), 1, MPI_UNSIGNED_CHAR, buffer,
               MPI_BUFFER_SIZE, position, MPI_COMM_WORLD);
      MPI_Pack(&(edge->vertex1), 1, MPI_UNSIGNED_LONG, buffer,
               MPI_BUFFER_SIZE, position, MPI_COMM_WORLD);
      MPI_Pack(&(edge->vertex2), 1, MPI_UNSIGNED_LONG, buffer,
               MPI_BUFFER_SIZE, position, MPI_COMM_WORLD);
      PackLabel(edge->label, parameters->labelList, buffer, position);
   }
}


//---------------------------------------------------------------------------
// NAME: PackLabel
//
// INPUTS: (ULONG index) - index of label in label list
//         (LabelList *labelList) - label list
//         (char *buffer) - MPI message buffer
//         (int *position) - position at end of buffer (by reference)
//
// RETURN: (void)
//
// PURPOSE: Packs label information into the MPI message buffer.
//---------------------------------------------------------------------------

void PackLabel(ULONG index, LabelList *labelList, char *buffer,
               int *position)
{
   UCHAR labelType;
   char *labelString;
   double labelValue;
   int labelLength;

   labelType = labelList->labels[index].labelType;
   MPI_Pack(&labelType, 1, MPI_UNSIGNED_CHAR, buffer, MPI_BUFFER_SIZE,
            position, MPI_COMM_WORLD);
   switch(labelType) 
   {
      case STRING_LABEL:
         labelString = labelList->labels[index].labelValue.stringLabel;
         labelLength = strlen(labelString);
         MPI_Pack(&labelLength, 1, MPI_INT, buffer, MPI_BUFFER_SIZE,
                  position, MPI_COMM_WORLD);
         MPI_Pack(labelString, labelLength + 1, MPI_CHAR, buffer,
                  MPI_BUFFER_SIZE, position, MPI_COMM_WORLD);
         break;
      case NUMERIC_LABEL:
         labelValue = labelList->labels[index].labelValue.numericLabel;
         MPI_Pack(&labelValue, 1, MPI_DOUBLE, buffer, MPI_BUFFER_SIZE,
                  position, MPI_COMM_WORLD);
         break;
      default:
         break;
   }
}


//---------------------------------------------------------------------------
// NAME: UnpackSubstructure
//
// INPUTS: (char *buffer) - buffer holding packed substructure
//         (int *position) - position in buffer to begin unpacking
//         (Parameters *parameters)
//
// RETURN: (Substructure *) - pointer to unpacked substructure
//
// PURPOSE: Unpacks the substructure in the given MPI message buffer.  The
// reference to position is set to the end of the buffer.  Note that sub
// may be NULL.  No instances are passed.
//---------------------------------------------------------------------------

Substructure *UnpackSubstructure(char *buffer, int *position,
                                 Parameters *parameters)
{
   Substructure *sub;
   Graph *graph;
   double value;
   ULONG numInstances;
   ULONG numNegInstances;

   // unpack parameters of substructure
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &value, 1, MPI_DOUBLE, 
              MPI_COMM_WORLD);
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &numInstances, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &numNegInstances, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

   // unpack graph (may be NULL)
   graph = UnpackGraph(buffer, position, parameters);

   // create and return substructure (NULL if graph is NULL)
   sub = NULL;
   if (graph != NULL) 
   {
      sub = AllocateSub();
      sub->definition = graph;
      sub->value = value;
      sub->numInstances = numInstances;
      sub->numNegInstances = numNegInstances;
      sub->instances = NULL;
      sub->negInstances = NULL;
   }
   return sub;
}


//---------------------------------------------------------------------------
// NAME: UnpackGraph
//
// INPUTS: (char *buffer) - MPI message buffer
//         (int *position) - position of end of buffer (by reference)
//         (Parameters *parameters)
//
// RETURN: (Graph *) - pointer to graph unpacked from message buffer
//
// PURPOSE: Unpack and return the graph in the given MPI message buffer.
// If no vertices in the graph, then return NULL.
//---------------------------------------------------------------------------

Graph *UnpackGraph(char *buffer, int *position, Parameters *parameters)
{
   Graph *graph;
   ULONG numVertices;
   ULONG numEdges;
   ULONG v, e;
   ULONG labelIndex;
   ULONG vertex1;
   ULONG vertex2;
   BOOLEAN directed;

   // unpack number of vertices and edges
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &numVertices, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &numEdges, 1,
              MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

   // unpack vertices and edges of graph (graph = NULL if no vertices)
   graph = NULL;
   if (numVertices > 0) 
   {
      graph = AllocateGraph(numVertices, numEdges);
      // unpack and store vertex information in graph
      for (v = 0; v < numVertices; v++) 
      {
         labelIndex = UnpackLabel(buffer, position, parameters->labelList);
         graph->vertices[v].label = labelIndex;
         graph->vertices[v].numEdges = 0;
         graph->vertices[v].edges = NULL;
         graph->vertices[v].map = VERTEX_UNMAPPED;
         graph->vertices[v].used = FALSE;
      }
      // unpack and store edge information in graph
      for (e = 0; e < numEdges; e++) 
      {
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &directed, 1,
                    MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &vertex1, 1,
         MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &vertex2, 1,
         MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
         labelIndex = UnpackLabel(buffer, position, parameters->labelList);
         graph->edges[e].vertex1 = vertex1;
         graph->edges[e].vertex2 = vertex2;
         graph->edges[e].label = labelIndex;
         graph->edges[e].directed = directed;
         graph->edges[e].used = FALSE;
         // add edge index to edge index array of both vertices
         AddEdgeToVertices(graph, e);
      }
   }
   return graph;
}


//---------------------------------------------------------------------------
// NAME: UnpackLabel
//
// INPUTS: (char *buffer) - MPI message buffer
//         (int *position) - position at end of buffer (by reference)
//         (LabelList *labelList) - label list
//
// RETURN: (ULONG) - index into label list of unpacked label
//
// PURPOSE: Unpacks a label from the given MPI message buffer and returns
// the label's index into the label list.  If the label does not exist,
// then adds it to the label list.
//---------------------------------------------------------------------------

ULONG UnpackLabel(char *buffer, int *position, LabelList *labelList)
{
   UCHAR labelType;
   char labelString[TOKEN_LEN];
   double labelValue;
   int labelLength;
   Label label;

   MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &labelType, 1,
              MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);

   switch(labelType) 
   {
      case STRING_LABEL:
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &labelLength, 1,
                    MPI_INT, MPI_COMM_WORLD);
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, labelString,
                    labelLength + 1, MPI_CHAR, MPI_COMM_WORLD);
         label.labelType = STRING_LABEL;
         label.labelValue.stringLabel = labelString;
         break;

      case NUMERIC_LABEL:
         MPI_Unpack(buffer, MPI_BUFFER_SIZE, position, &labelValue, 1,
                    MPI_DOUBLE, MPI_COMM_WORLD);
         label.labelType = NUMERIC_LABEL;
         label.labelValue.numericLabel = labelValue;
         break;

      default:
         break;
   }
   return StoreLabel(&label, labelList);
}
