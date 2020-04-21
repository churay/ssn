#ifndef SSN_MODES_T_H
#define SSN_MODES_T_H

#include "ssn.h"
#include "ssn_consts.h"
#include "consts.h"

namespace ssn {

namespace mode {
    namespace game {
        bool32_t init( ssn::state_t* pState );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace title {
        bool32_t init( ssn::state_t* pState );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace score {
        bool32_t init( ssn::state_t* pState );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace reset {
        bool32_t init( ssn::state_t* pState );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }
}

}

#endif
