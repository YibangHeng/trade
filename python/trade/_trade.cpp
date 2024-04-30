#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "trade/trade.h"

namespace py = pybind11;

namespace trade
{

PYBIND11_MODULE(_trade, m)
{
    m.doc() = "Python Bindings for trade";
}

} // namespace trade
