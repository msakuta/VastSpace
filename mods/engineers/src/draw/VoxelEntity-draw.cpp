/// \file
/// \brief Implementation of VoxelEntity class's graphics.
#define NOMINMAX
#include "VoxelEntity.h"
#include "draw/WarDraw.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/material.h"
#include "glstack.h"
#include "draw/mqoadapt.h"
#include "draw/VBO.h"
#include "draw/MarchingCube.h"

extern "C"{
#include "clib/mathdef.h"
}

#include "cpplib/crc32.h"

static const int maxViewDistance = 10.;


void VoxelEntity::draw(WarDraw *wd){

	const Vec3i inf = VoxelEntity::real2ind(wd->vw->pos - this->pos);
	std::vector<CellVolume*> changed;
	double noiseHeight = getNoiseHeight();
	double baseHeight = getBaseHeight();
	double maxHeight = baseHeight + noiseHeight * 2.; // Theoretical limit height
	int maxCellCount = int(maxHeight / getCellWidth() / CELLSIZE) + 1;
	int radius = maxViewDistance / getCellWidth() / CELLSIZE;
	for (int ix = -maxCellCount; ix <= maxCellCount; ix++){
		for (int iy = -maxCellCount; iy <= maxCellCount; iy++){
			for (int iz = -maxCellCount; iz <= maxCellCount; iz++){
				Vec3i ci(ix, iy, iz);
				if(volume.find(ci) == volume.end()){
					volume[ci] = CellVolume(this, ci);
					CellVolume &cv = volume[ci];
					cv.initialize(ci);
					changed.push_back(&volume[ci]);
				}
			}
		}
	}

	for(std::vector<CellVolume*>::iterator it = changed.begin(); it != changed.end(); it++)
		(*it)->updateCache();

	vboInitBuffers();

	{
		GLmatrix glm;
		GLattrib gla(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
		Mat4d mat;
		transform(mat);
		glMultMatrixd(mat);

		// Set some ambient illumination for showing features in the shadow.
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Vec4<float>(.25,.25,.25,1.));
		// Reset specular and shininess parameters to prevent strange shading in GeForce boards with shaders.
		glMaterialfv(GL_FRONT, GL_SPECULAR, Vec4f(0,0,0,1));
		glMaterialfv(GL_FRONT, GL_SHININESS, Vec3f(50,0,0));

		const ShaderBind *sb = NULL;
		if(wd->shadowMap){
			sb = wd->shadowMap->getShader();
			if(sb){
				sb->enableTextures(true, false);
			}
		}

		Cell::Type celltype = Cell::Air;

		const double maxViewCells = maxViewDistance / getCellWidth();

		for(VoxelEntity::VolumeMap::iterator it = this->volume.begin(); it != this->volume.end(); it++){
			const Vec3i &key = it->first;
			CellVolume &cv = it->second;

			initTexLists();

			// If all content is air, skip drawing
			if(cv.getSolidCount() == 0)
				continue;

			// Examine if intersects or included in viewing frustum
//			if(!FrustumCheck(xmvc(VoxelEntity::ind2real(key * CELLSIZE).cast<float>()), xmvc(World::ind2real((key + Vec3i(1,1,1)) * CELLSIZE).cast<float>()), frustum))
//				continue;

			// Cull too far CellVolumes
			if ((key[0] + 1) * CELLSIZE + maxViewCells < inf[0])
				continue;
			if (inf[0] < key[0] * CELLSIZE - maxViewCells)
				continue;
			if ((key[1] + 1) * CELLSIZE + maxViewCells < inf[1])
				continue;
			if (inf[1] < key[1] * CELLSIZE - maxViewCells)
				continue;
			if ((key[2] + 1) * CELLSIZE + maxViewCells < inf[2])
				continue;
			if (inf[2] < key[2] * CELLSIZE - maxViewCells)
				continue;

			if(cv.vboDirty){
				VertexMap vertexMap(operator<);
				std::vector<VERTEX> vlist[cv.Num_Mat];
				std::vector<GLuint> vidx[cv.Num_Mat];
				cv.modeledCells.clear();

				for(int ix = 0; ix < CELLSIZE; ix++){
					for(int iz = 0; iz < CELLSIZE; iz++){
						// This detail culling is not much effective.
						//if (bf.Contains(new BoundingBox(ind2real(keyindex + new Vec3i(ix, kv.Value.scanLines[ix, iz, 0], iz)), ind2real(keyindex + new Vec3i(ix + 1, kv.Value.scanLines[ix, iz, 1] + 1, iz + 1)))) != ContainmentType.Disjoint)
						const int (&scanLines)[CELLSIZE][CELLSIZE][2] = cv.getScanLines();
						for (int iy = 0; iy < CELLSIZE; iy++){

							// Cull too far Cells
/*							if (cv(ix, iy, iz).getType() == Cell::Air)
								continue;
							if (maxViewCells < abs(ix + it->first[0] * CELLSIZE - inf[0]))
								continue;
							if (maxViewCells < abs(iy + it->first[1] * CELLSIZE - inf[1]))
								continue;
							if (maxViewCells < abs(iz + it->first[2] * CELLSIZE - inf[2]))
								continue;*/

							// If the Cell is buried under ground, it's no use examining each face of the Cell.
//							if(6 <= cv(ix, iy, iz).getAdjacents())
//								continue;

							const Cell &cell = cv(ix, iy, iz);
							Vec3i posInVolume(ix, iy, iz);

							drawCell(cell, posInVolume + it->first * CELLSIZE, celltype, &cv, posInVolume, vlist, vidx, &vertexMap);
						}
					}
				}

				// Smooth shading by averaging normal vectors for vertices at the same position
				for(auto vi : vertexMap){
					// If this vertex is not shared, do not bother trying to calculate average of normals.
					if(vi.second.size() <= 1)
						continue;

					Vec3d sum(0,0,0);
					for(auto v : vi.second)
						sum += vlist[v.material][v.index].norm;

					// If the summed normal is zero, leave the normal vector
					if(0 < sum.slen()){
						sum.normin();

						// Replace the normal with the average of vertex normals in the same position.
						for(auto v : vi.second)
							vlist[v.material][v.index].norm = sum;
					}
				}

				for(int i = 0; i < cv.Num_Mat; i++){
					if(0 < vidx[i].size()){
						if(cv.vbo[i] == 0)
							glGenBuffers(1, &cv.vbo[i]);
						if(cv.vboIdx[i] == 0)
							glGenBuffers(1, &cv.vboIdx[i]);

						glBindBuffer(GL_ARRAY_BUFFER, cv.vbo[i]);
						glBufferData(GL_ARRAY_BUFFER, vlist[i].size() * sizeof(vlist[i][0]), &vlist[i].front(), GL_STATIC_DRAW);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cv.vboIdx[i]);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, vidx[i].size() * sizeof(vidx[i][0]), &vidx[i].front(), GL_STATIC_DRAW);
					}

					// We can forget about the actual buffer content, but must remember the number of elements
					// in the buffer in order to perform glDrawElements() afterwards.
					cv.vlistCounts[i] = vlist[i].size();
					cv.vidxCounts[i] = vidx[i].size();
				}

				cv.vboDirty = false;
			}

			for(int i = 0; i < cv.Num_Mat; i++){
				if(cv.vboIdx[i]){
					GLmatrix glm;

					switch(i){
					case CellVolume::MatRock: glCallList(texlist_rock); break;
					case CellVolume::MatIron: glCallList(texlist_iron); break;
					case CellVolume::MatArmor: glCallList(texlist_armor); break;
					}

					glScaled(getCellWidth(), getCellWidth(), getCellWidth());

					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_NORMAL_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					glBindBuffer(GL_ARRAY_BUFFER, cv.vbo[i]);
					glVertexPointer(3, GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, pos)); // Vertex array
					glNormalPointer(GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, norm)); // Normal array
					glTexCoordPointer(2, GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, tex)); // Texture coordinates array

					// Index array
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cv.vboIdx[i]);

					glDrawElements(GL_TRIANGLES, cv.vidxCounts[i], GL_UNSIGNED_INT, 0);

					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}

			for(auto ipos : cv.modeledCells)
				drawCell(cv.cell(ipos), ipos + it->first * CELLSIZE, celltype, &cv, ipos);
		}
	}
}

void VoxelEntity::drawtra(WarDraw *wd){
	if(previewCell.getType() != Cell::Air){
		GLmatrix glm;
		GLattrib gla(GL_TEXTURE_BIT | GL_CURRENT_BIT);
		Mat4d mat;
		transform(mat);
		glMultMatrixd(mat);

		glColor4f(1,1,1,0.5f);

		glEnable(GL_CULL_FACE);

		initTexLists();

		const ShaderBind *sb = NULL;
		if(wd->shadowMap){
			sb = wd->shadowMap->getShader();
			if(sb){
				sb->enableTextures(true, false);
			}
		}

		Cell::Type celltype = Cell::Air;

		drawCell(previewCell, previewCellPos, celltype);
	}
}

/// \brief An internal function to draw a single cell in VoxelEntity
/// \param cell A cell to draw
/// \param pos Position vector (integral) in voxel space
/// \param celltype A rendering state variable to minimize number of texture switching
/// \param cv A CellVolume object for adjacent cell checking
/// \param posInVolume The position vector in the CellVolume object
/// \param vlist A pointer to an array with size 3 of vertex buffer for list of vertices, can be NULL for direct drawing
/// \param vidx A pointer to an array with size 3 of index buffer for list of primitives
void VoxelEntity::drawCell(const Cell &cell, const Vec3i &pos, Cell::Type &celltype, const CellVolume *cv, const Vec3i &posInVolume,
						   std::vector<VERTEX> *vlist, std::vector<GLuint> *vidx, VertexMap *vertexMap)const
{
	static const Vec3d slopeNormal = Vec3d(1,1,0).norm();
	static const Vec3d cornerNormal = Vec3d(1,1,1).norm();

	// Create vertex buffer
	static const VERTEX vertices[] =
	{
		{Vec3d(0, 0, 0), Vec3d(0, -1, 0), Vec2d(0, 0)},
		{Vec3d(0, 0, 1), Vec3d(0, -1, 0), Vec2d(0, 1)},
		{Vec3d(1, 0, 1), Vec3d(0, -1, 0), Vec2d(1, 1)},
		{Vec3d(1, 0, 0), Vec3d(0, -1, 0), Vec2d(1, 0)},

		{Vec3d(0, 1, 0), Vec3d(0, 1, 0), Vec2d(0, 0)},
		{Vec3d(1, 1, 0), Vec3d(0, 1, 0), Vec2d(1, 0)},
		{Vec3d(1, 1, 1), Vec3d(0, 1, 0), Vec2d(1, 1)},
		{Vec3d(0, 1, 1), Vec3d(0, 1, 0), Vec2d(0, 1)},

		{Vec3d(0, 0, 0), Vec3d(0, 0, -1), Vec2d(0, 0)},
		{Vec3d(1, 0, 0), Vec3d(0, 0, -1), Vec2d(1, 0)},
		{Vec3d(1, 1, 0), Vec3d(0, 0, -1), Vec2d(1, 1)},
		{Vec3d(0, 1, 0), Vec3d(0, 0, -1), Vec2d(0, 1)},

		{Vec3d(0, 0, 1), Vec3d(0, 0, 1), Vec2d(0, 0)},
		{Vec3d(0, 1, 1), Vec3d(0, 0, 1), Vec2d(0, 1)},
		{Vec3d(1, 1, 1), Vec3d(0, 0, 1), Vec2d(1, 1)},
		{Vec3d(1, 0, 1), Vec3d(0, 0, 1), Vec2d(1, 0)},

		{Vec3d(0, 0, 0), Vec3d(-1, 0, 0), Vec2d(0, 0)},
		{Vec3d(0, 1, 0), Vec3d(-1, 0, 0), Vec2d(1, 0)},
		{Vec3d(0, 1, 1), Vec3d(-1, 0, 0), Vec2d(1, 1)},
		{Vec3d(0, 0, 1), Vec3d(-1, 0, 0), Vec2d(0, 1)},

		{Vec3d(1, 0, 0), Vec3d(1, 0, 0), Vec2d(0, 0)},
		{Vec3d(1, 0, 1), Vec3d(1, 0, 0), Vec2d(0, 1)},
		{Vec3d(1, 1, 1), Vec3d(1, 0, 0), Vec2d(1, 1)},
		{Vec3d(1, 1, 0), Vec3d(1, 0, 0), Vec2d(1, 0)},

		{Vec3d(1, 0, 0), slopeNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 0), slopeNormal, Vec2d(0, 1)},
		{Vec3d(0, 1, 1), slopeNormal, Vec2d(1, 1)},
		{Vec3d(1, 0, 1), slopeNormal, Vec2d(1, 0)},

		{Vec3d(1, 0, 0), cornerNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 0), cornerNormal, Vec2d(0, 1)},
		{Vec3d(0, 0, 1), cornerNormal, Vec2d(1, 1)},

		{Vec3d(1, 1, 0), cornerNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 1), cornerNormal, Vec2d(0, 1)},
		{Vec3d(1, 0, 1), cornerNormal, Vec2d(1, 1)},
	};

	static const unsigned indices[] =
	{
		2,1,0,
		0,3,2,

		6,5,4,
		4,7,6,

		10,9,8,
		8,11,10,

		14,13,12,
		12,15,14,

		18,17,16,
		16,19,18,

		22,21,20,
		20,23,22
	};

	static const unsigned slopeIndices[] = {
		2,1,0,
		0,3,2,

		18,17,16,
		16,19,18,

		11,9,8,

		15,13,12,

		24,25,26,
		26,27,24
	};

	static const unsigned cornerIndices[] = {
		0,3,1,

		16,19,17,

		8,11,9,

		28,29,30,
	};

	static const unsigned invCornerIndices[] = {
		2,1,0,
		0,3,2,

		10,9,8,
		8,11,10,

		18,17,16,
		16,19,18,

		20,23,21,

		4,7,5,

		12,15,13,

		31,32,33,
	};

	int material = 0;
	if(!vlist){
		if(celltype != cell.getType()){
			celltype = cell.getType();
			switch(celltype){
			case Cell::Rock: glCallList(texlist_rock); break;
			case Cell::Iron: glCallList(texlist_iron); break;
			case Cell::Armor:
			case Cell::ArmorSlope:
			case Cell::ArmorCorner:
			case Cell::ArmorInvCorner: glCallList(texlist_armor); break;
			case Cell::Occupied: return; // The occupied cell is not rendered because another cell should draw it.
			}
		}
	}
	else{
		celltype = cell.getType();
		switch(celltype){
		case Cell::Rock: material = CellVolume::MatRock; break;
		case Cell::Iron: material = CellVolume::MatIron; break;
		case Cell::Armor:
		case Cell::ArmorSlope:
		case Cell::ArmorCorner:
		case Cell::ArmorInvCorner: material = CellVolume::MatArmor; break;
		case Cell::Occupied: return; // The occupied cell is not rendered because another cell should draw it.
		}
	}

	Mat4d rotmat;

	auto drawIndexedGeneral = [cv, vlist, vidx, &pos, material, &rotmat](int cnt, int base, const unsigned indices[]){
		for(int i = base; i < base + cnt; i++){
			if(vlist){
				std::vector<VERTEX> &vl = vlist[material];
				VERTEX vtx = vertices[indices[i]];
				vtx.pos = rotmat.vp3(vtx.pos) + pos.cast<double>();
				vtx.norm = rotmat.dvp3(vtx.norm);
				bool match = false;
				for(int j = MAX(0, vl.size() - 16); j < vl.size(); j++){
					if(vl[j] == vtx){
						vidx[material].push_back(j);
						match = true;
					}
				}
				if(!match){
					vidx[material].push_back(vl.size());
					vl.push_back(vtx);
				}
			}
			else{
				glNormal3dv(vertices[indices[i]].norm);
				glTexCoord2dv(vertices[indices[i]].tex);
				glVertex3dv(vertices[indices[i]].pos);
			}
		}
	};

	auto drawIndexed = [&](int cnt, int base){
		drawIndexedGeneral(cnt, base, indices);
	};

	auto drawIndexedSlope = [&](int cnt, int base){
		drawIndexedGeneral(cnt, base, slopeIndices);
	};

	GLmatrix glm;
	Vec3d ofs(pos.cast<double>());
	if(!vlist){
		glScaled(getCellWidth(), getCellWidth(), getCellWidth());
		glTranslated(ofs[0], ofs[1], ofs[2]);

		if(cell.getRotation()){
			glTranslated(0.5, 0.5, 0.5);
			if(char c = (cell.getRotation() & 0x3))
				glRotatef(90 * c, 1, 0, 0);
			if(char c = ((cell.getRotation() >> 2) & 0x3))
				glRotatef(90 * c, 0, 1, 0);
			if(char c = ((cell.getRotation() >> 4) & 0x3))
				glRotatef(90 * c, 0, 0, 1);
			glTranslated(-0.5, -0.5, -0.5);
		}

		if(cell.getType() == Cell::Engine){
			static Model *model = LoadMQOModel("models/block_engine.mqo");
			static const double modelScale = 0.01;
			if(model){
				glTranslated(0.5, 0.5, 0.5);
				glScaled(modelScale, modelScale, modelScale);
				DrawMQOPose(model, NULL);
			}
			return;
		}
	}
	else{
		if(cell.getRotation()){
			Mat4d mid = mat4_u.translate(0.5, 0.5, 0.5);
			if(char c = (cell.getRotation() & 0x3))
				mid = mid.rotx(M_PI / 2. * c);
			if(char c = ((cell.getRotation() >> 2) & 0x3))
				mid = mid.roty(M_PI / 2. * c);
			if(char c = ((cell.getRotation() >> 4) & 0x3))
				mid = mid.rotz(M_PI / 2. * c);
			rotmat = mid.translate(-0.5, -0.5, -0.5);
		}
		else
			rotmat = mat4_u;

		if(cell.getType() == Cell::Engine){
			cv->modeledCells.push_back(posInVolume);
			return;
		}
	}

	bool x0 = false;
	bool x1 = false;
	bool y0 = false;
	bool y1 = false;
	bool z0 = false;
	bool z1 = false;
	if(cv){
		int ix = posInVolume[0];
		int iy = posInVolume[1];
		int iz = posInVolume[2];
		x0 = !(*cv)(ix - 1, iy, iz).isTranslucent();
		x1 = !(*cv)(ix + 1, iy, iz).isTranslucent();
		y0 = !(*cv)(ix, iy - 1, iz).isTranslucent();
		y1 = !(*cv)(ix, iy + 1, iz).isTranslucent();
		z0 = !(*cv)(ix, iy, iz - 1).isTranslucent();
		z1 = !(*cv)(ix, iy, iz + 1).isTranslucent();
	}

	if(!vlist)
		glBegin(GL_TRIANGLES);
	if(cell.getType() == Cell::ArmorSlope){
		drawIndexedSlope(numof(slopeIndices), 0);
	}
	else if(cell.getType() == Cell::ArmorCorner){
		drawIndexedGeneral(numof(cornerIndices), 0, cornerIndices);
	}
	else if(cell.getType() == Cell::ArmorInvCorner){
		drawIndexedGeneral(numof(invCornerIndices), 0, invCornerIndices);
	}
	else if(vlist){
		using namespace MarchingCube;
		GRIDCELL gcell;
		for(int k = 0; k < 8; k++){
			gcell.p[k] = vertexOffsets[k].cast<double>();
			bool solid = (*cv).cell(posInVolume + vertexOffsets[k]).isTranslucent();
			if(solid){
				Vec3i posbuf = pos + vertexOffsets[k];
				gcell.val[k] = RandomSequence(crc32(&posbuf, sizeof posbuf)).nextd() + 0.5;
			}
			else
				gcell.val[k] = 0;
		}
		TRIANGLE tris[5];
		int ntris = Polygonise(gcell, 0.5, tris);
		for(int i = 0; i < ntris; i++){
			TRIANGLE &tri = tris[i];
			Vec3d v01 = tri.p[1] - tri.p[0];
			Vec3d v02 = tri.p[2] - tri.p[0];
			Vec3d normal = v02.vp(v01);
			if(0 < normal.slen())
				normal.normin();
			else
				normal = Vec3d(0,1,0);
			for(int j = 2; 0 <= j; j--){
				std::vector<VERTEX> &vl = vlist[material];
				VERTEX vtx;
				vtx.pos = tris[i].p[j] + pos.cast<double>();
				vtx.norm = normal;
				vtx.tex = Vec2d(j == 1 || j == 2, j == 2);
				bool match = false;
				for(int k = std::max(0, (int)vl.size() - 16); k < vl.size(); k++){
					if(vl[k] == vtx){
						vidx[material].push_back(k);
						match = true;
					}
				}
				if(!match){
					vidx[material].push_back(vl.size());
					if(vertexMap){
						VertexMapValue vmv;
						vmv.material = material;
						vmv.index = vl.size();
						(*vertexMap)[vtx.pos].push_back(vmv);
					}
					vl.push_back(vtx);
				}
			}
		}
	}
	else{
		if(!x0 && !x1 && !y0 && !y1){
			drawIndexed(36, 0);
		}
		else{
			if(!x0)
				drawIndexed(2 * 3, 8 * 3);
			if(!x1)
				drawIndexed(2 * 3, 10 * 3);
			if(!y0)
				drawIndexed(2 * 3, 0);
			if(!y1)
				drawIndexed(2 * 3, 2 * 3);
			if(!z0)
				drawIndexed(2 * 3, 4 * 3);
			if(!z1)
				drawIndexed(2 * 3, 6 * 3);
		}
	}
	if(!vlist)
		glEnd();

}

void VoxelEntity::initTexLists(){
	if(!texlist_init){
		texlist_init = true;
		texlist_rock = CallCacheBitmap("rock.jpg", texRockName, NULL, NULL);
		texlist_iron = CallCacheBitmap("iron.jpg", texIronName, NULL, NULL);
		texlist_armor = CallCacheBitmap("armor.png", texArmorName, NULL, NULL);
	}
}
