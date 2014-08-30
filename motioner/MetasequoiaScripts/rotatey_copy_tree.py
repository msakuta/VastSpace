##
# This script copies a subtree of selected Metasequoia object tree
# with rotated vertices around Y axis.
# Tested in Metasequoia 3.1.
# The copied objects have names with varying last character which indicate
# index of rotation angle.
# It reserves materials, too.
# It's handy to use this script to update skeleton models having rotational
# symmetry, e.g. helicopter rotor blades.

import math

# Convert a MQVector to a string representation
def v2s(v):
	return "(" + str(v.x) + ", " + str(v.y) + ", " + str(v.z) + ")"

# Convert a Vertex Index List to an ordinary integer list.
# We need this because MQFace.index yields a MQFaceIndexArray class,
# not an integer list, contrary to the documentation.
def vil2list(l):
	ilist = []
	for k in range(len(l)):
		ilist.append(l[k])
	return ilist

# Prepare four-fold rotation matrix prior to the main loop.
matrices = []
for o in range(4):
	phase = o * math.pi / 2
	matrices.append([math.cos(phase), math.sin(phase),
		-math.sin(phase), math.cos(phase)])

doc = MQSystem.getDocument()

MQSystem.println(" doc: " + str(len(doc.object)))

startDepth = doc.object[doc.currentObjectIndex].depth

# Find the parent object of the selected object.
# Note that doc.object[doc.currentObjectIndex-1] is not necessarily the parent
# because it can be a sibling.
parent = None
for i in range(doc.currentObjectIndex):
	o = doc.object[i]
	if o.depth == startDepth - 1:
		parent = o
		MQSystem.println(" current object's parent: " + o.name)

def rotateCopyObj(obj):
	MQSystem.println(" obj.name: " + obj.name)
	MQSystem.println(" obj.depth: " + str(obj.depth))

	# We want to copy a subtree of selected object, so do not
	# scan after the layer depth goes higher than the selected one.
	if doc.currentObjectIndex < i and obj.depth <= startDepth:
		return

	MQSystem.println(" obj.numFace: " + str(obj.numFace))

	# The naming convention is assumed that the last character of object name
	# is a digit indicating rotation position.  We'll remember the base
	# object's one and increment as we proceed copying rotated versions.
	if obj.name[-1].isdigit():
		basenum = int(obj.name[-1])
	else:
		return

	for o in range(1,4):
		# Create copy of the rotated object
		newobj = MQSystem.newObject()
		newobj.name = obj.name[:-1] + str(basenum + o)
		MQSystem.println(" new object name: " + newobj.name)

		mat = matrices[o]

		vl = obj.vertex
#		MQSystem.println(" obj.vertex: " + str(vl))
		for v in vl:
			p = v.getPos()
#			MQSystem.println(" v: " + str(p))

			# Save rotated copy of the source vertices
			newx = p.x * mat[0] + p.z * mat[1]
			newz = p.x * mat[2] + p.z * mat[3]
			newobj.addVertex(newx, p.y, newz)

#		MQSystem.println(" obj.face: " + str(obj.face))
		for j in range(len(obj.face)):
			face = obj.face[j]

			ilist = vil2list(face.index)
			newobj.addFace(ilist)

			# Obtain the face just added and assign the same material as
			# the copy source to it
			newface = newobj.face[-1]
			newface.material = face.material

			# Debug output: list vertices in face
#			MQSystem.println("  face: " + str(len(face.index)))
#			for k in face.index:
#				vertPos = newobj.vertex[k].getPos()
#				vertPos = obj.vertex[k].getPos()
#				MQSystem.println("    vertex[%d]: %s" % (k, vertPos))

			# Notify that the face is an edge.
			if face.numVertex == 2:
				MQSystem.println("  found edge at " + str(j) + " which seems like a bone")

		# TODO: Reserve layer structure of the copy source
		doc.addObject(newobj, parent)

rotateCopyObj(doc.object[doc.currentObjectIndex])
