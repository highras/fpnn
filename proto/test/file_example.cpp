#include "FPReader.h"
#include "FPWriter.h"
#include "FPMessage.h"
#include "JSONConvert.h"
#include "msec.h"
#include <sys/time.h>
#include <unistd.h>
#include <iostream>     // std::cout
#include <fstream>

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[]){
	//file must be exist, TODO
	
	FPQWriter qw(6, "FileTest");
	qw.param("int", 123456);
	qw.paramFile("filea", "./bin/a");
	
	qw.param("string", "abcd");
	qw.paramFile("filec", "./bin/c.jpg");
	qw.param("string2", "abcd");
	qw.paramFile("filee", "./bin/e");
	

	FPQuestPtr quest = qw.take();
	FPQReader qr(quest);

	{
		FileSystemUtil::FileAttrs attrs;
		bool ret = qr.wantFile("filea", attrs);
		cout<<"ret: "<<ret<<endl;
		cout<<"name: "<<attrs.name<<endl;
		cout<<"contentLen: "<<attrs.content.size()<<endl;
		cout<<"sign: "<<attrs.sign<<endl;
		cout<<"ext: "<<attrs.ext<<endl;
		cout<<"size: "<<attrs.size<<endl;
		cout<<"atime: "<<attrs.atime<<endl;
		cout<<"mtime: "<<attrs.mtime<<endl;
		cout<<"ctime: "<<attrs.ctime<<endl;
		cout<<endl;
	}

	{
		FileSystemUtil::FileAttrs attrs;
		bool ret = qr.wantFile("filec", attrs);
		cout<<"ret: "<<ret<<endl;
		cout<<"name: "<<attrs.name<<endl;
		cout<<"contentLen: "<<attrs.content.size()<<endl;
		cout<<"sign: "<<attrs.sign<<endl;
		cout<<"ext: "<<attrs.ext<<endl;
		cout<<"size: "<<attrs.size<<endl;
		cout<<"atime: "<<attrs.atime<<endl;
		cout<<"mtime: "<<attrs.mtime<<endl;
		cout<<"ctime: "<<attrs.ctime<<endl;
		cout<<endl;
	}

	{
		FileSystemUtil::FileAttrs attrs;
		bool ret = qr.wantFile("filee", attrs);
		cout<<"ret: "<<ret<<endl;
		cout<<"name: "<<attrs.name<<endl;
		cout<<"contentLen: "<<attrs.content.size()<<endl;
		cout<<"sign: "<<attrs.sign<<endl;
		cout<<"ext: "<<attrs.ext<<endl;
		cout<<"size: "<<attrs.size<<endl;
		cout<<"atime: "<<attrs.atime<<endl;
		cout<<"mtime: "<<attrs.mtime<<endl;
		cout<<"ctime: "<<attrs.ctime<<endl;
		cout<<endl;
	}

	string content;
	FileSystemUtil::readFileContent("bin/e", content);

	cout<<"File Len:"<<content.size()<<endl;

	size_t test_count = 20000;
	struct timeval start;
	struct timeval end;
	gettimeofday(&start, NULL);
	for(size_t i = 0; i < test_count; ++i){
		FPQWriter qw(1, "FileQuestTest");
		qw.param("file", content);
		//qw.paramBinary("file", content.data(), content.size());
		//FPQuestPtr quest = qw.take();
	}

	gettimeofday(&end, NULL);

	int64_t dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FILE FPQWrite: speed="<< test_count*1000/dur << "/s"<<endl;

	FPQWriter qww(1, "FileQuestTest");
	qww.param("file", content);
	FPQuestPtr questw = qww.take();
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		FPQReader qr(questw);
		//FPQuestPtr quest = qw.take();
	}

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FILE FPQRead: speed="<< test_count*1000/dur << "/s"<<endl;


	return 0;
}

