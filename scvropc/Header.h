////////////////////////////////////////////////////////////////////////
// project: SimpleOPCClient
// version: 2 
//
// subject: this is a simple OPC Client that read the value of an item
// from an OPC server.
//
// file: SimpleOPCClient.h
//
// writer: Philippe GRAS - CERN / EP
//




#ifndef SIMPLE_OPC_CLIENT_H
#define SIMPLE_OPC_CLIENT_H

IOPCServer* InstantiateServer(wchar_t ServerName[]);
IOPCItemMgt * AddTheGroup(IOPCServer * pIOPCServer, LPWSTR Groupname, OPCHANDLE & hServerGroup);
OPCHANDLE AddTheItem(IOPCItemMgt* pIOPCItemMgt, LPWSTR ItemID, VARTYPE ItemType);
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue);
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem);
void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup);
int InitOPC(void);
void EndOPC(void);
int RunOPC(void);
double ReadTheItem(int item);
void ReadAllItems(double * pval);

#endif // SIMPLE_OPC_CLIENT_H not defined