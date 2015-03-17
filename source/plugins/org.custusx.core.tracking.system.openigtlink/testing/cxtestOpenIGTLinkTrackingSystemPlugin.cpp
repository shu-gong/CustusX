/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/
#include "catch.hpp"

#include "cxOpenIGTLinkTrackingSystemService.h"
#include "cxtestQueuedSignalListener.h"
#include "cxUtilHelpers.h"
#include "cxReporter.h"

TEST_CASE("OpenIGTLinkTrackingSystemService: Check that the service can be created and destroyed", "[unit][plugins][org.custusx.core.tracking.system.openigtlink]")
{
    cx::OpenIGTLinkTrackingSystemServicePtr service = cx::OpenIGTLinkTrackingSystemServicePtr(new cx::OpenIGTLinkTrackingSystemService());
    REQUIRE(service);
    CHECK(service.unique());
    service.reset();
}

TEST_CASE("OpenIGTLinkTrackingSystemService: Check that the can connect and stream from a server", "[manual][plugins][org.custusx.core.tracking.system.openigtlink]")
{
    cx::OpenIGTLinkTrackingSystemServicePtr service = cx::OpenIGTLinkTrackingSystemServicePtr(new cx::OpenIGTLinkTrackingSystemService());
    REQUIRE(service);
    CHECK(service.unique());

    REQUIRE(service->getState() == cx::Tool::tsCONFIGURED);

    service->setState(cx::Tool::tsINITIALIZED);
    REQUIRE(cxtest::waitForQueuedSignal(service.get(), SIGNAL(stateChanged())));
    REQUIRE(service->getState() == cx::Tool::tsINITIALIZED);

    service->setState(cx::Tool::tsTRACKING);
    REQUIRE(cxtest::waitForQueuedSignal(service.get(), SIGNAL(stateChanged())));

    //TODO change to get a packages signal????
    cx::sleep_ms(1000); //just let some packages arrive

    service->setState(cx::Tool::tsINITIALIZED);
    REQUIRE(cxtest::waitForQueuedSignal(service.get(), SIGNAL(stateChanged())));

    //service->setState(cx::Tool::tsNONE);
    //REQUIRE(cxtest::waitForQueuedSignal(service.get(), SIGNAL(stateChanged())));
}
