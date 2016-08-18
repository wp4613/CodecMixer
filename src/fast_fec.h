
#ifndef _FEC_FAST_FEC_H
#define _FEC_FAST_FEC_H

#include <stdint.h>

namespace fec {

    // keep same memory layout with FecRepo::Param
    struct Param {
        int K, T, KP, S, H, W, L, P, P1, U, B, J;

        /*
        * K - the src symbol num; T - the symbol size
        */
        static void GetParam(int K, int T, Param &param);

        inline int ESIToISI(int esi) const {
            if (esi < K)
                return esi;
            return KP + (esi - K);
        }

        inline int ISIToESI(int isi) const {
            if (isi < K)
                return isi;
            return isi - (KP - K);
        }
    };

    struct Symbol {
        int esi;
        uint8_t *data; // data length is defined in param.T
    };

    // this is the intermediate symbols, generated by encoder and decoder
    // used to generate unlimited number of repair symbols
    struct LTSymbols {
        Param param;
        uint8_t *symbolVec; // layout - | aligned rowidx | aligned row data |
        /*
        * for the LTSymbols generated from src symbols, rowNum == param.L
        * for recovered from repair symbols, rowNum == the number of symbol input for decoding
        */
        int rowNum;
        int length;
    };

    class IAllocator {
    public:
        enum {
            PurposeUnknown,
            PurposeLT,
            PurposeTmpMatrix,
        };
    
        virtual ~IAllocator() {}
        // alignment : 0 - no needed; 16 - align to 16 bytes
        virtual void* AllocFecBuffer(int size, int alignment, int purpose = PurposeUnknown) = 0;
        virtual void  FreeFecBuffer(void *buffer, int purpose = PurposeUnknown) = 0;
    };

    class FastFec {
    public:
        /*
        * the data must be padded with 0s to be the length of K * T
        */
        static void GenLTSymbols(const Param &param, const uint8_t* data, int paddedLen, 
            IAllocator *allocator, LTSymbols &ltSymbols);
            
        static void GenLTSymbols(const Param &param, const uint8_t* srcSymData[], int srcSymNum, 
            IAllocator *allocator, LTSymbols &ltSymbols);

        /*
        * the returning data length is the stride of T
        */
        static const uint8_t* GetLTSymbolData(const LTSymbols &ltSymbols, int idx);

        /*
        * the symbols must be ordered by esi, the last src symbol must be padded by 0s
        */
        static bool TryDecode(const Param &param, const Symbol *symbols, int numOfSymbol, 
            IAllocator *allocator, LTSymbols &ltSymbols);
        
        /*
         * symbols - start from esi 0 to maxESI [0 maxESI), if the slot is nullptr, that symbols is not valid
         * symbolNum - valid symbol number in symbols array, it must be greater than K
         * maxESI - the max esi + 1, it is also the size of symbols array
         */ 
        static bool TryDecode(const Param &param, const uint8_t* symbols[], int maxESI, int validSymbolNum,
            IAllocator *allocator, LTSymbols &ltSymbols);

        /*
        * gen symbol from intermediate symbols, data length is larger than param.T
        * data should be aligned to 16bytes, length must be multiple of 16bytes !
        */
        static void RecoverSymbol(const LTSymbols &ltSymbols, 
            int esi, uint8_t *data, int length);
    };
}

#endif
