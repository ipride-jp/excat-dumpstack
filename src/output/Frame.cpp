#include "Frame.h"
#include "../common/Define.h"
#include "Method.h"
#include "../common/HelperFunc.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

const char *Frame::ATTR_DEPTH = "Depth";
const char *Frame::TAG_FRAME = "Frame";

Frame::Frame(DumpParam *param)
{
	this->param = param;
	
	init();
}

Frame::~Frame()
{

}

void Frame::init()
{
	DOMDocument *doc = param->parent->getOwnerDocument();

	/*
	XMLCh tempStr[MAX_BUF_LEN];
	XMLString::transcode(TAG_FRAME, tempStr, MAX_BUF_LEN - 1);
	DOMElement *frameNode = doc->createElement(tempStr);
	
	char buf[MAX_BUF_LEN];
	sprintf(buf, "%d", param->depth);
	HelperFunc::addAttrToDOMElement(frameNode, ATTR_DEPTH, buf);

	param->parent->appendChild(frameNode);
	param->parent = frameNode;
	*/

	//Method method(param);
}