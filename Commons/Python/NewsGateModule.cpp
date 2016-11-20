/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Commons/NewsGateModule.cpp
 * @author Karen Arutyunov
 * $Id:$
 */
#include <Python.h>

#include <El/Python/Module.hpp>

namespace NewsGate
{
  El::Python::Module newsgate_module(
    "newsgate",
    "Module containing NewsGate library types.",
    true);
}
