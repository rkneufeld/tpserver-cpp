#ifndef FRAME_H
#define FRAME_H

typedef enum {
	ft_Invalid = -1,
	ft_Connect = 0,
	ft_OK = 1,
	ft_Login = 2,
	ft_Fail = 3,
	ft_Get_Object = 4,
	ft_Object = 5,
	ft_Get_Order = 6,
	ft_Order = 7,
	ft_Add_Order = 8,
	ft_Remove_Order = 9,
	ft_Describe_Order = 10,
	ft_Order_Description = 11,
	ft_Max
} FrameType;

//class std::string;

class Frame {

      public:
	Frame();
	Frame(Frame & rhs);
	~Frame();

	Frame operator=(Frame & rhs);

	char *getPacket();
	FrameType getType();
	int getLength();
	char *getData();

	int setHeader(char *newhead);
	bool setType(FrameType nt);
	bool setData(char *newdata, int dlen);

	bool packString(char *str);
	//bool packString(std::string str);
	bool packInt(int val);
	bool packInt64(long long val);

	// uses these functions with care
	int getUnpackOffset();
	bool setUnpackOffset(int newoffset);

	int unpackInt();
	char *unpackString();
	long long unpackInt64();

	void createFailFrame(int code, char *reason);

      private:
	 FrameType type;
	int length;
	char *data;
	int unpackptr;

};

#endif
