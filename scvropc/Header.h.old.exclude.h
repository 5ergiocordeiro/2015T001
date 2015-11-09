////////////////////////////////////////////////////////////////////////
// project: SimpleOPCClient
// version: 1 
//
// subject: this is a very simple OPC Client that instantiate then
// release the IOPCServer interface of an OPC server.
//
// file: SimpleOPCClient.h
//
// writer: Philippe GRAS - CERN / EP
//



#ifndef SIMPLE_OPC_CLIENT_H
#define SIMPLE_OPC_CLIENT_H

IOPCServer* InstantiateServer(wchar_t ServerName[]);

#endif // SIMPLE_OPC_CLIENT_H not defined