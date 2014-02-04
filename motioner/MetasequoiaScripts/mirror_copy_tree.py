##
# This script copies a subtree of a Metasequoia object tree
# with mirrored vertices along X axis of the source.
# Tested in Metasequoia 3.1.
# The copied objects have names prefixed by 'r' if the source object
# name begins with 'l', and vice versa.
# It reserves materials, too.
# It's handy to use this script to update skeleton model of one side
# to the opposite side automatically.


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

doc = MQSystem.getDocument()

len(doc.object)

MQSystem.println(" doc: " + str(len(doc.object)))

startDepth = doc.object[doc.currentObjectIndex].depth

MQSystem.println(" obj.name: " + str(range(doc.currentObjectIndex, len(doc.object))))

prevobj = None

for i in range(doc.currentObjectIndex, len(doc.object)):
	obj = doc.object[i]

	# The doc.object list occasionally contains None as element,
	# which seems to mark the end of valid object list.
	if obj == None:
		break

	MQSystem.println(" obj.name: " + obj.name)
	MQSystem.println(" obj.depth: " + str(obj.depth))

	# We want to copy a subtree of selected object, so do not
	# scan after the layer depth goes higher than the selected one.
	if doc.currentObjectIndex < i and obj.depth <= startDepth:
		break

	MQSystem.println(" obj.numFace: " + str(obj.numFace))

	# Create copy of the mirrored object
	newobj = MQSystem.newObject()

	# The naming convention is assumed that the name has 'r' or 'l' as the
	# first character, denoting right or left counterpart, respectively.
	if obj.name[0] == 'r':
		newobj.name = "l" + obj.name[1:]
	else:
		newobj.name = "r" + obj.name[1:]

	vl = obj.vertex
	for v in vl:
#		MQSystem.println(" obj.vertex: " + str(vl))
#		MQSystem.println(" v: " + str(k))
#		v = vl[k].getPos()
		p = v.getPos()
#		MQSystem.println(" v: " + v2s(p))
		newobj.addVertex(-p.x, p.y, p.z)

	for j in range(len(obj.face)):
		face = obj.face[j]
#		MQSystem.println("  face[" + str(j) + "]:" + str(face.index))

		ilist = vil2list(face.index)
		ilist.reverse() # Reverse order of vertices for mirroring
#		MQSystem.println("  " + str(ilist))
		newobj.addFace(ilist)

		# Obtain the face just added and assign the same material as
		# the copy source to it
		newface = newobj.face[-1]
		newface.material = face.material

#		for k in face.index:
#			vertPos = newobj.vertex[k].getPos()
#			vertPos = obj.vertex[face.index[k]].getPos()
#			MQSystem.println("    vertex[" + str(k)
#				+ "]:" + str(face.index[k])
#				+ ": " + v2s(vertPos))

#		if face.numVertex == 2:
#			MQSystem.println("  found edge at " + str(j) + " which seems like a bone")

	# TODO: Reserve layer structure of the copy source
	if prevobj == None:
		doc.addObject(newobj)
	else:
		doc.addObject(newobj, prevobj)
	prevobj = newobj

