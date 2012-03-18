#ifndef LIBMOTION_H
#define LIBMOTION_H
/** \file
 * \brief Definitions and implementation of Motion* classes.
 *
 * The implementation uses C++ STL only.
 * STL saved great amount of redundant codes that would be necessary to
 * manage memory usage of containers, compared to the class library's
 * predecessor, ysdnm.
 *
 * Planned to be usable by C sources.
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <clib/suf/suf.h>
#include <clib/avec3.h>
#include <clib/aquat.h>
#include <clib/timemeas.h>

#ifdef __cplusplus
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include "dstring.h"

#include <map>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdlib.h> // atoi
#endif


// Type definitions varies in languages
#ifdef __cplusplus
typedef Vec3d MotVec3;
typedef Quatd MotQuat;
typedef Vec3<sufcoord> sufvector;
#else
typedef avec3_t MotVec3;
typedef aquat_t MotQuat;
typedef sufcoord sufvector[3];
#endif

#ifdef __cplusplus

/// The exception type used by this class library.
class MotionException : std::exception{
	virtual const char *what()const throw(){
		return "Exception in motion class libraries";
	}
};

#endif



/// Single bone modifier (variation).
/// It can be accessed from C.
struct MotionNode{
	MotQuat rot; ///< Rotation modifier.
	MotVec3 pos; ///< Position modifier, applied after rotation.
	float visible; ///< Visibility factor. Visible if > 0.
};

#ifdef __cplusplus

/// Generic 3-D model modifier.
///
/// It's merely a set of information nodes that describes which and how bones of a model should turn.
///
/// Nodes are identified by names (strings), so arbitrary model could apply any MotionPose.
/// This will enable re-utilization of motion data.
struct MotionPose{
#if 0
	typedef std::map<gltestp::dstring, MotionNode> NodeMap;
#else

	/// Experimental std::map-compatible class that maps a dstring to a MotionNode.
	///
	/// Its implementation is so simple that is just an array of std::pairs,
	/// but its heap memory usage is expected to be lower than std::map.
	/// Specifically, getting an iterator or finding a key won't cost a heap
	/// memory allocation, provided the key argument is prepared as a dstring, too.
	///
	/// In reality, its performance is not much different compared to std::map, but
	/// speed of debug build is closer to that of release build.
	class NodeMap{
		typedef std::pair<gltestp::dstring, MotionNode> Node; ///< Internal node type.
		Node **nodes; ///< Array of pointers to the nodes.
		int n; ///< Count of nodes in the array nodes.
	public:

		/// Iterator type is just a pointer to the node array in the source class.
		/// Templating const and non-const iterator saves lines.
		template<typename T> class titerator{
			T **p;
		public:
			titerator(T **a) : p(a){}
			T &operator*(){return **p;}
			T *operator->(){return *p;}
			titerator &operator++(){p++; return *this;}
			titerator &operator++(int){++p; return *this;}
			bool operator==(const titerator &o)const{return p == o.p;}
			bool operator!=(const titerator &o)const{return p != o.p;}
			friend class NodeMap;
		};
		typedef titerator<Node> iterator;
		typedef titerator<const Node> const_iterator;

		NodeMap() : nodes(NULL), n(0){}
		NodeMap(const NodeMap &o) : nodes(new Node*[o.n]), n(o.n){
			for(int i = 0; i < n; i++)
				nodes[i] = new Node(*o.nodes[i]);
		}
		~NodeMap(){
			for(int i = 0; i < n; i++)
				delete nodes[i];
			delete[] nodes;
		}

		NodeMap &operator=(const NodeMap &o){
			this->NodeMap::~NodeMap();
			n = o.n;
			nodes = new Node*[o.n];
			for(int i = 0; i < n; i++)
				nodes[i] = new Node(*o.nodes[i]);
			return *this;
		}

		iterator begin(){return nodes;}
		iterator end(){return &nodes[n];}
		const_iterator begin()const{return const_cast<const Node**>(nodes);}
		const_iterator end()const{return const_cast<const Node**>(&nodes[n]);}
		iterator find(const gltestp::dstring &a){
			for(int i = 0; i < n; i++) if(nodes[i]->first == a)
				return &nodes[i];
			return end();
		}
		const_iterator find(const gltestp::dstring &a)const{
			return const_cast<const Node**>(const_cast<NodeMap*>(this)->find(a).p);
		}
		MotionNode &operator[](const gltestp::dstring &a){
			iterator it = find(a);
			if(it != end())
				return it->second;
			else
				return push_back(a).second;
		}
		Node &push_back(const gltestp::dstring &a){
			Node **newnodes = new Node*[n+1];
			memcpy(newnodes, nodes, n * sizeof *nodes);
			delete[] nodes;
			nodes = newnodes;
			nodes[n] = new Node(a, MotionNode());
			n++;
			return *nodes[n-1];
		}

		size_t size()const{return n;}
	};
#endif
	typedef NodeMap::iterator iterator;
	typedef NodeMap::const_iterator const_iterator;

	MotionPose *next;
	NodeMap nodes;


	MotionPose() : next(NULL){
	}

	MotionPose &operator=(const MotionPose &o){
		next = o.next;
		nodes = o.nodes;
		return *this;
	}

	MotionNode *addNode(const char *name);
	MotionPose &amplify(double f);
};

#else /* not __cplusplus */

typedef struct MotionPose MotionPose;

#endif


#ifdef __cplusplus

/// A motion object represents a sequence of movements of model bones.
///
/// This sequence is expressed in a list of keyframes.
struct Motion{

	/// Internal class that represents a frame of motion sequence that serves as basis of
	/// interpolation.
	struct keyframe : public MotionPose{
		double dt;
		keyframe(double dt = 0.) : dt(dt){}
		keyframe(const keyframe &o) : MotionPose(o), dt(o.dt){
		}
		keyframe &operator=(const keyframe &o){
			MotionPose::operator=(o);
			dt = o.dt;
			return *this;
		}
	};

	/// The sole member of Motion is the keyframe list.
	std::vector<keyframe> kfl;

	Motion(){}

	Motion(std::istream &is){
		load(is);
	}

	Motion(const std::string &file){
		std::ifstream is(file.c_str());
		if(is.fail())
			throw MotionException();
		load(is);
	}

	double totalTime()const{
		double ret = 0.;
		for(int i = 0; i < kfl.size(); i++)
			ret += kfl[i].dt;
		return ret;
	}
	keyframe &addKeyframe(double dt){
		kfl.push_back(keyframe(dt));
		return kfl.back();
	}
	keyframe &insertKeyframe(double dt, int index){
		assert(0 <= index && index < kfl.size());
		return *kfl.insert(kfl.begin() + index, keyframe(dt));
	}
	keyframe &getKeyframe(double time){
		if(kfl.empty())
			throw MotionException();
		return kfl[getKeyframeIndex(time)];
	}
	int getKeyframeIndex(double time){
		if(time < 0.)
			time = 0.;
		int i;
		const int nkfl = kfl.size();
		for(i = 0; i < nkfl && kfl[i].dt <= time; time -= kfl[i].dt, i++);
		return nkfl <= i ? nkfl - 1 : i;
	}
	double getTimeOfKeyframe(int keyframe){
		double ret = 0;
		const int nkfl = kfl.size();
		for(int i = 0; i < keyframe && i < nkfl; i++)
			ret += kfl[i].dt;
		return ret;
	}
	MotionPose &interpolate(MotionPose &v, double time);
	void save(std::ostream&);
	void save(const std::string&);
	void save2(std::ostream&);
	void save2(const std::string&);
	bool load(std::istream&);
	bool load(std::string);
};


#else

typedef struct Motion Motion;

#endif





#ifdef __cplusplus


//-----------------------------------------------------------------------------
//     Implementation
//-----------------------------------------------------------------------------


namespace cpplib{
/// Template specialization for spherical linear interpolation of Quaternions.
template<> inline Quat<double> Quat<double>::slerp(const Quatd &q, const Quatd &r, const double t){
	tt ret;
	double qr = q.a[0] * r.a[0] + q.a[1] * r.a[1] + q.a[2] * r.a[2] + q.a[3] * r.a[3];
	double ss = 1.0 - qr * qr;
  
	if (ss <= 0.0) {
		return q;
	}
	else if(q == r){
		return q;
	}
	else {
		double sp = ::sqrt(ss);
		double ph = ::acos(qr);
		double pt = ph * t;
		double t1 = ::sin(pt) / sp;
		double t0 = ::sin(ph - pt) / sp;

		// Long path case
		if(qr < 0)
			t1 *= -1;

		return tt(
			q.a[0] * t0 + r.a[0] * t1,
			q.a[1] * t0 + r.a[1] * t1,
			q.a[2] * t0 + r.a[2] * t1,
			q.a[3] * t0 + r.a[3] * t1);
	}
}
}


/// Returns existing node or create a new one if it doesn't.
///
/// Simply returning &nodes[name] would work, too.
inline MotionNode *MotionPose::addNode(const char *name){
	iterator it = nodes.find(name);
	if(it == nodes.end()){
		// MotionNode does not initialize its content by the constructor, so we
		// take care of those uninitialized memory.
		MotionNode &ret = nodes[name];
		ret.pos = Vec3d(0,0,0);
		ret.rot = Quatd(0,0,0,1);
		ret.visible = 1.;
		return &ret;
	}
	else
		return &nodes[name];
}


/// Amplify this modifier's offset and rotation by given factor.
///
/// The argument should be between 0 and 1, but values greater than 1
/// might work too.
inline MotionPose &MotionPose::amplify(double f){
	for(iterator it = nodes.begin(); it != nodes.end(); it++){
		it->second.rot = Quatd::slerp(Quatd(0,0,0,1), it->second.rot, f);
		it->second.pos *= f;
	}
	return *this;
}


/// Interpolate the keyframes to produce a pose that should smoothly connects adjacent keyframes,
/// provided time argument is continuous.
///
/// This function requires returning MotionPose object allocated and passed as an argument.
inline MotionPose &Motion::interpolate(MotionPose &v, double time){
	if(!this || !kfl.size())
		return v;
//		timemeas_t tm;
//		TimeMeasStart(&tm);
	double f;
	int i;
	if(time < 0.)
		time = 0.;
	int nkfl = kfl.size();
	for(i = 0; i < nkfl && kfl[i].dt <= time; time -= kfl[i].dt, i++);
	keyframe &prev = kfl[i < nkfl ? i : nkfl-1], &next = kfl[i+1 < nkfl ? i+1 : nkfl-1];
	v = prev;
	if(time == 0. || &prev == &next)
		return v;
	for(MotionPose::iterator it = prev.nodes.begin(); it != prev.nodes.end(); it++){
		MotionPose::iterator it2 = next.nodes.find(it->first);
		if(it2 != next.nodes.end()){
			MotionNode node;
			node.rot = Quatd::slerp(it->second.rot, it2->second.rot, time / prev.dt);
			node.pos = it->second.pos * (1. - time / prev.dt) + it2->second.pos * time / prev.dt;
			node.visible = it->second.visible * (1. - time / prev.dt) + it2->second.visible * time / prev.dt;
			v.nodes[it->first] = node;
		}
	}
//		std::cerr << "interpolate " << TimeMeasLap(&tm) << std::endl;
	return v;
}



/// Serialize and save the contents of the motion in textual form and output them to the given stream.
inline void Motion::save(std::ostream &os){
	os << "ysdnm_motion " << kfl.size() << std::endl;
	os << "{" << std::endl;
	for(int i = 0; i < kfl.size(); i++){
		os << "\tkeyframe " << kfl[i].nodes.size() << " " << kfl[i].dt << std::endl;
		os << "\t{" << std::endl;
		MotionPose::iterator it = kfl[i].nodes.begin();
		for(; it != kfl[i].nodes.end(); it++){
			os << "\t\t\"" << it->first << "\"";
			for(int k = 0; k < 4; k++)
				os << " " << std::setprecision(15) << it->second.rot[k];
			for(int k = 0; k < 3; k++)
				os << " " << std::setprecision(15) << it->second.pos[k];
			os << std::endl;
		}
		os << "\t}" << std::endl;
	}
	os << "}" << std::endl;
}

/// Save to given file path, overwriting existing one.
inline void Motion::save(const std::string &file){
	std::ofstream ofs(file.c_str());
	save(ofs);
}


/// Serialize and save the object in a text file format version 2.
inline void Motion::save2(std::ostream &os){
	os << "model_motion " << kfl.size() << std::endl;
	os << "{" << std::endl;
	for(int i = 0; i < kfl.size(); i++){
		os << "\tkeyframe " << kfl[i].nodes.size() << " " << kfl[i].dt << std::endl;
		os << "\t{" << std::endl;
		MotionPose::iterator it = kfl[i].nodes.begin();
		for(; it != kfl[i].nodes.end(); it++){
			os << "\t\t\"" <<it->first << "\"";
			for(int k = 0; k < 4; k++)
				os << " " << std::setprecision(15) << it->second.rot[k];
			for(int k = 0; k < 3; k++)
				os << " " << std::setprecision(15) << it->second.pos[k];
			os << " " << it->second.visible;
			os << std::endl;
		}
		os << "\t}" << std::endl;
	}
	os << "}" << std::endl;
}

/// Save to given file path in version 2, overwriting existing one.
inline void Motion::save2(const std::string &file){
	std::ofstream ofs(file.c_str());
	save2(ofs);
}


/// Try to unserialize or load from a text stream.
///
/// New or old version can be loaded.
inline bool Motion::load(std::istream &is){
	try{
	std::string line;
	char buf[512];
	is.getline(buf, sizeof buf);
	line = std::string(buf);

	// Obtain and remember very first token of the file, that's version string.
	int firstspace = line.find(' ');
	std::string firsttoken = line.substr(0, firstspace);
	int version = firsttoken == "model_motion";
	int nkfl = atoi(line.substr(firstspace).c_str());
	kfl.reserve(nkfl);

	is.getline(buf, sizeof buf);
	for(int i = 0; i < nkfl; i++){
		keyframe newkfl;
		is.getline(buf, sizeof buf);
		int nnodes;
		double dt;
		sscanf(buf, "\tkeyframe %d %lg", &nnodes, &dt);
		newkfl.dt = dt;
		is.getline(buf, sizeof buf);

		for(int j = 0; j < nnodes; j++){
			is.getline(buf, sizeof buf);
			char *name0 = ::strchr(buf, '"')+1;
			char *name1 = ::strchr(name0, '"');
			gltestp::dstring newname(name0, name1 - name0);
			std::istringstream iss(name1+1);

			MotionNode node;
			for(int k = 0; k < 4; k++)
				iss >> node.rot[k];
			for(int k = 0; k < 3; k++)
				iss >> node.pos[k];
			node.rot.normin();
			if(version)
				iss >> node.visible;
			else
				node.visible = 1.;

			newkfl.nodes[newname] = node;
		}

		kfl.push_back(newkfl);

		is.getline(buf, sizeof buf);
	}
	is.getline(buf, sizeof buf);
	}
	catch(...){
		return false;
	}
	return true;
}

#endif


#endif
