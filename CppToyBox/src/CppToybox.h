#pragma once

// defined common define macro

#define CT_PLACEMENT_NEW(buffer, Type) new(buffer) Type
#define CT_PLACEMENT_DELETE(p) _CT_PLACEMENT_DELETE(p)

template< typename T >
void _CT_PLACEMENT_DELETE(T* p) { p->~T(); }