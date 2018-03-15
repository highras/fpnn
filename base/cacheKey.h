#ifndef CACHEKEY_H_
#define CACHEKEY_H_

#include "jenkins.h"
#include "HashFunctor.h"

namespace fpnn {

struct PIDXID
{
	int64_t _pid;  //
	int64_t _xid;  //

	PIDXID(){
		_pid = 0;
		_xid = 0;
	}

	PIDXID(int32_t pid, int64_t uid){
		_pid = pid;
		_xid = uid;
	}

	PIDXID(const PIDXID& k){
		_pid = k._pid;
		_xid = k._xid;
	}
	
	bool operator==(const struct PIDXID& key){
		return _xid == key._xid && _pid == key._pid;
	}

	bool operator<(const struct PIDXID& right) const{
		if(_xid != right._xid) 
			return _xid < right._xid;
		return _pid < right._pid;
	}

	unsigned int hash() const{
		return jenkins_hash(this, sizeof(*this), 0);
	}
};

}

#endif

