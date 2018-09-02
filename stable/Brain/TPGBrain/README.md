### TPG BRAIN notes
** there may be an issue where programY -> nodeX -> programY. In this case, this node is always
safe from mutation and deletion. a check whenever a program mutates to an existing node should
be made to insure that it is not pointing at a node that points at the program. NOTE: the
nodeX -> programY -> nodeX issue is addessed **

** while the serialize function is written here, the deserialize function has not been. This is
just a matter of labor required... it's a pain. The current serialization saves only elements
needed for a given org (i.e. brain) but it would be esier if all nodes were saved. Beyond this
makeing sure that this is working with MABE namespace would also need to be addressed (it
might be OKay, it might not **


TPG (Tangled Program Graph) Brains are collections
of Nodes (that point to lists of Programs), Programs (that point at one Nodes or one Atomics)
and Atomics (outputs). A Node which does not have any Programs pointing at it is a
root node, and each root defines a brain. To run a TPG we start at the root node and
run each program in that nodes list and follow the pointer in Program which generated the
greates value. If the Program points to an Atomic, this is the brains output (the brain
update is done) if the Program points to another Node then we run that Nodes Programs.
If we visit a Node more then once, we follow the Program with the next highest value.
If a Nodes Program list has been fully exprored, the brain returns a 0 output.


TPG Brains share componets. Aside from root Nodes, elements in a TPG may be shared by
many TPG (i.e. a TPG does not own it's parts!). When TPG are mutated, only root Nodes are
subject to mutation. When a root Node is mutated a copy of the node is created and mutated.
Possible mutations are:
change program list.
change a program in the list (this will result in a copy of the program being created)
  - change to program formula
  - change to program node pointer (if a root node is pointed at, it stops being a root)
  - change to atomic

If a program changes it's pointer so that is points at a root node, that node is no longer
a root, and is protected form mutation.
If a program stops pointing a a node and no other program is pointing at that node, the
node is then a root node. that node defines a brain and is subject to mutation.

In order to evolve TPG a special optimizer (TPGOptimizer) must be used.
TPG brains do not generate lieages.
