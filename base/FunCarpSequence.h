#ifndef FUNCARPSEQUNCE_H
#define	FUNCARPSEQUNCE_H
#define __STDC_LIMIT_MACROS
#include <vector>
#include <string>
#include "carp.h"
#include <stdint.h>

namespace fpnn {
    class FunCarpSequence{
    public:    
        FunCarpSequence(const std::vector<std::string>& server_ids, uint32_t keymask = 0);
        FunCarpSequence(const std::vector<std::string>& server_ids, const std::vector<uint32_t>& weights, uint32_t keymask = 0); 
        ~FunCarpSequence();
        size_t size() const { 
            if(_carp)
                return carp_total(_carp);
            else 
                return 0;
        }
        uint32_t mask() const { return _mask; }
    
        int which(const char* key);
        int which(const std::string& key);
        int which(const int64_t key);
        size_t sequence(const char* key, size_t num, std::vector<size_t>& seq);
        size_t sequence(const std::string& key, size_t num, std::vector<size_t>& seq);
        size_t sequence(const int64_t key, size_t num, std::vector<size_t>& seq);
    private:
        carp_t *_carp;
        uint32_t _mask;
    };
}
#endif	/* FUNCARPSEQUNCE_H */

