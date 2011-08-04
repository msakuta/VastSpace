#ifndef MSG_GETCOVERPOINTSMESSAGE_H
#define MSG_GETCOVERPOINTSMESSAGE_H
#include "Message.h"
#include <squirrel.h>
#include <cpplib/vec3.h>
#include <stddef.h>
#include <vector>


struct CoverPoint{
	Vec3d pos;
	Quatd rot;
	enum Type{LeftEdge, RightEdge, TopEdge} type;
};

class EXPORT CoverPointVector : public std::vector<CoverPoint>{
};


struct EXPORT GetCoverPointsMessage : public Message{
	typedef Message st;
	static MessageRegister<GetCoverPointsMessage> messageRegister;
	static MessageID sid;
	virtual MessageID id()const;
	virtual bool derived(MessageID)const;
	GetCoverPointsMessage(){}
	GetCoverPointsMessage(HSQUIRRELVM v, Entity &e);
	Vec3d org;
	double radius;
	CoverPointVector cpv;
};


#endif
