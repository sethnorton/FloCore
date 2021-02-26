/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2021] VulcanForms Incorporated
*  All Rights Reserved.
*
*  "VulcanForms", "Vulcan", "Fusing the Future"
*       are trademarks of VulcanForms, Inc.
*
* NOTICE:  All information contained herein is, and remains
* the property of VulcanForms Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to VulcandForms Incorporated
* and its suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from VulcanForms Incorporated.
*/


//The execution backend for VulcanForms Inc business logic.
//
//The core can be run locally or on a server, and provided with a dependency graph of components to exectute, it will make execution decisions based on compute resources available to it.
//
//The general execution flow might be:
//
//    Discover available compute units attached by network.
//    Discover available components on each compute unit and locally.
//    Wait for client connection.
//    When client connects, send list of available components.
//    Receive dependency graph or sub-graph from client.
//    Transfer all data from URIs that are attached to the dependency graph to the appropriate compute units.
//    Start component executables (could map many components to one executable with flags).
//    Track progress and respond to any client requests about progress
//    Collect results and wait for client to request.
//    Transfer results back to client.
//    Goto 3.
#include <stdio.h>
#include <iostream>


int main(int* cargs, char** vargs)
{
    printf("Hello world\n");

    std::cin.ignore();
}
