SUBDUE 5 README

SUBDUE is a graph-based data mining system. See copytight.txt for
distribution and warranty information.

Installation

  In the 'src' directory, type 'make' and then 'make install'.  This will put 
  everything in the 'bin' directory.  If you want the MPI version of SUBDUE 
  (and have MPI properly installed on your system), you can type 
  'make mpi_subdue' and then 'make install_mpi'. You can type 'make clean' in 
  the 'src' directory to remove the object files and executables there.

Documentation

  There is a Users Manual (located in the /docs directory) that contains 
  details and examples.  Several sample input graphs have been included in the 
  'graphs' directory.

Change History:

Version 5.2.2 (released 06/06/2011)
- Fixed bug with not initializing incremental parameter in mdl_main.c
- Moved some graph memory allocations parameters to inside Graph structure
- Fixed bugs in mpi_main.c related to incremental parameter issues
- Fixed initialization of instance->mapping in AllocateInstance
- Fixed initial condition in NewEdgeMatch to prevent checking label of
  unmapped vertex
- Changed instance printing so that vertex numbers align with those in
  input file when using multiple examples
- Added check for edges referring to a vertex 0 in the input file

Version 5.2.1 (released 05/30/06)
- removed manual.html file in lieu of more comprehensive PDF Users Manual.
- updated documentation to reflect new SUBDUE copyrights and references.
- casted EOF checks to (char) to make code compatible in AIX environment.
- fixed list of substructure instances to always include the generating 
  instance.
- modified DiscoverSubs to not evaluate a substructure with zero 
  negative instances when the evaluation method is SetCover.
- fixed the predefined substructure output to start numbering vertices at 1.

Version 5.2 (released 01/30/06)
- added incremental option (-inc) to process multiple graph input files 
  incrementally.
- added compress option (-compress) that allows for the writing of the 
  compressed data to a file.

Version 5.1.5 (released 12/20/05)
- added mapping to substructure instances so that quick comparisons can be done,
  avoiding more expensive calls to check for subgraph isomorphisms.
- added check on number of instances, whereby substructures with only one 
  instance (and no negative instances) are no longer extended.

Version 5.1.4 (released 08/29/05)
- fix implemented for defect that was uncovered whereby, in certain situations,
  non-isomorphic instances were being added to the list of instances for
  the corresponding substructure.  This defect existed in both the 5.1.2 and
  5.1.3 releases.
- added output that displays, for each of the best substructure, the number of
  of unique examples, both positive and negative (for output level 5 only).
- updated code to reflect coding standards

Version 5.1.3 (released 05/02/05)
- added checks in AddPosInstancesToSub so that the system will not compare 
  two positive instances that are either identical or were compared previously, 
  when the threshold is 0.0 (performance improvement).
- removed "Variables" and "Relations" from parameters printout.
- removed SUBDUE_HOME from Makefile, and added #define to cvtest to look for
  SUBDUE executable in local directory.
- increased value in constants LIST_SIZE_INC and FILE_NAME_LEN.
- added Users Manual.
- updated "Installation" instructions and "Documentation" in this file.

Version 5.1.2 (released 01/13/05)
- added call to NewEdgeMatch for runs where the threshold is 0.0.  This checks
  to see if the newly added edge (and vertex) match exactly, and thus
  avoiding an expensive call to GraphMatch (InexactGraphMatch).
- changed running time output to use CPU time (instead of wall-clock time).

Version 5.1.1 (released 07/01/04)
- outputs TP/TN/FP/FN statistics for cross-validation runs
- Makefile sets SUBDUE_HOME, which is passed to all compilations, but only
  utilized by cvtest_main.c for now

Version 5.1.0 (released 12/31/03)
- added ability to learn recursive substructures (-recursion option)
  - extensive changes, mostly in extend.c
- added 'used' flag to label list

Version 5.0.9 (not released)
- added new 'cvtest' program that performs a cross-validation experiment
  for a given set of examples (see docs/manual.html for details)
- added new 'test' program that tests the ability of a set of substructures
  to correctly classify a set of graphs (see docs/manual.html for details)
- changed default limit setting from Size(G+)/2 to E(G+)/2
- added CountSubs function; used to give accurate count of best subs
- fixed memory leak in ExtendSubs, freeing newSub if already on
  extendedSubs list

Version 5.0.8 (released 8/26/03)
- added a new program called 'gprune' that allows the user to prune a graph
  by removing all vertices and edges with a certain label, along with any
  vertices and edges left hanging as a result
- changed MATCH_SEARCH_THRESHOLD_EXPONENT to 4.0, instead of 0.0 which
  was taking too long
- fixed a bug in discover.c:GetInitialSubs() which caused some negative
  instances to be missed
- changed main discovery loop to add new subs only if strictly better
  rather than equal to or better
- in discover.c:GetInitialSubs() pruned initial substructures that have
  only one instance in positive graph
- changed main.c:main() to print number of unique labels for each iteration
- changed graphops.c:ReadToken() to skip over carriage returns, in case
  input file uses DOS/Windows format
- replaced use of strcasecmp() in graphops.c with strcmp(), because some
  other C compiler string libraries don't have strcasecmp(); of course,
  now all tokens (e.g., 'v', 'e', etc.) are case sensitive
- added fflush(stdout) to main discovery loop

Version 5.0.7 (released 7/4/03)
- major changes to enable MPI distributed processing support (see manual,
  mpi_main.c and mpi.c)
- double-quote delimited labels are printed without quotes in dot format
- changed default graph match behavior to no limit on number of mappings
  by setting MATCH_SEARCH_THRESHOLD_EXPONENT = 0.0 in subdue.h

Version 5.0.6 (released 2/24/03)
- changed design of graph matcher to accept a threshold and discard
  partial matches exceeding the threshold
- removed redundant checks for inserting unique substructures into
  lists of substructures
- removed redundant generation of substructures from instances
- changed "matchCost" field of instances to "minMatchCost"
- removed unused "mapping" field of instances

Version 5.0.5 (released 1/1/03)
- major changes to graphmatch.c to implement a heap for the mapping queue
- changes to dot.c and calling functions so that graph2dot can handle
  input graphs with positive and negative examples
- fixed error in SubListInsert() so that all eqi-valued substructures
  already on the list are compared to the new substructure to be inserted
- added check in main.c for no substructures returned from DiscoverSubs()
- changed stopping condition in main.c to no edges left instead of only
  single vertex left, and removed check that negative graph compressed
- added timings in seconds per iteration and total
- added output level 5 for printing all substructures considered
- when printing instances, also prints example number containing instance
- final best substructures are numbered in output
- bug fix in DeletedEdges of graphmatch.c to correctly handle self edges
- changed directory structure: bin, docs, src
- added SUBDUE logo to manual.html
- added ability to read "quoted" labels containing whitespace and the
  comment character (%)

Version 5.04 (released 10/05/02)
- Improved efficiency of FindInstances, and therefore also finding
  predefined substructures and the sgiso program
- SUBDUE has a new option '-out' that allows writing the best
  substructures of each iteration in a machine-readable format
- Added the ability to create dot format graphs (see manual.html)
  - Program 'subs2dot' works with the '-out' file of SUBDUE
    - This gives the ability to view cluster hierarchies
  - Program 'graph2dot' converts a SUBDUE-formatted graph to dot format
  - Programs 'mdl' and 'sgiso' now have -dot options
- Minor bug fixes
- Updates to this README file and the manual.html file

Version 5.03 (released 9/26/02)
- Added concept learning capability

Version 5.02 (released 9/17/02)
- Initial release


The SUBDUE Project has benefitted greatly from funding agencies and researchers
over the years, who are listed below:

Funding

Air Force Rome Laboratory (AFRL)
Defense Advanced Research Projects Agency (DARPA)
Department of Homeland Security (DHS)
National Aeronautics and Space Administration (NASA)
Naval Research Laboratory (NRL)
National Science Foundation (NSF)
Texas Higher Education Coordinating Board (THECB)

Researchers

Diane Cook
Larry Holder
Jeff Coble
Surnjani Djoko
Bill Eberle
Gehad Gelal
Jesus Gonzalez
Istvan Jonyer
Nikhil Ketkar

