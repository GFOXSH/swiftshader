//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Diagnostics.h"

#include "debug.h"
#include "InfoSink.h"
#include "preprocessor/SourceLocation.h"

TDiagnostics::TDiagnostics(TInfoSink& infoSink) :
    mShaderVersion(100),
    mInfoSink(infoSink),
    mNumErrors(0),
    mNumWarnings(0)
{
}

TDiagnostics::~TDiagnostics()
{
}

void TDiagnostics::setShaderVersion(int version)
{
    mShaderVersion = version;
}

void TDiagnostics::writeInfo(Severity severity,
                             const pp::SourceLocation& loc,
                             const std::string& reason,
                             const std::string& token,
                             const std::string& extra)
{
    TPrefixType prefix = EPrefixNone;
    switch (severity)
    {
      case PP_ERROR:
        ++mNumErrors;
        prefix = EPrefixError;
        break;
      case PP_WARNING:
        ++mNumWarnings;
        prefix = EPrefixWarning;
        break;
      default:
        UNREACHABLE(severity);
        break;
    }

    TInfoSinkBase& sink = mInfoSink.info;
    /* VC++ format: file(linenum) : error #: 'token' : extrainfo */
    sink.prefix(prefix);
    TSourceLoc sourceLoc;
    sourceLoc.first_file = sourceLoc.last_file = loc.file;
    sourceLoc.first_line = sourceLoc.last_line = loc.line;
    sink.location(sourceLoc);
    sink << "'" << token <<  "' : " << reason << " " << extra << "\n";
}

void TDiagnostics::writeDebug(const std::string& str)
{
    mInfoSink.debug << str;
}

void TDiagnostics::print(ID id,
                         const pp::SourceLocation& loc,
                         const std::string& text)
{
    writeInfo(severity(id), loc, message(id), text, "");
}
