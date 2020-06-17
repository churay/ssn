#ifndef SSN_MODES_T_H
#define SSN_MODES_T_H

#include "ssn.h"
#include "ssn_consts.h"
#include "consts.h"

namespace ssn {

namespace mode {
    namespace boot { constexpr static mode_e ID = -1; }
    namespace exit { constexpr static mode_e ID = -2; }

    namespace game {
        constexpr static mode_e ID = 0;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace select {
        constexpr static mode_e ID = 1;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace title {
        constexpr static mode_e ID = 2;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace bind {
        constexpr static mode_e ID = 5;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace score {
        constexpr static mode_e ID = 3;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }

    namespace reset {
        constexpr static mode_e ID = 4;
        bool32_t init( ssn::state_t*, ssn::input_t* );
        bool32_t update( ssn::state_t*, ssn::input_t*, const float64_t );
        bool32_t render( const ssn::state_t*, const ssn::input_t*, const ssn::output_t* );
    }
}

}

#endif
