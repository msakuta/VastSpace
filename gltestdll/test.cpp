#include "../serial.h"


class TestDerive : public Serializable{
	static const unsigned classid;
public:
	virtual const char *classname()const{return "TestDerive";}
};

const unsigned TestDerive::classid = registerClass("TestDerive", Conster<TestDerive>);

