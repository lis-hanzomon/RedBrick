/* 
 * This file is part of RedBrick.
 * Copyright (c) 2018 Link Information Systems Co., Ltd.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MH_DISCOVERY_PARSER_H
#define MH_DISCOVERY_PARSER_H

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

const int ACTION_TYPE_PROBE         = 1;
const int ACTION_TYPE_PROBE_MATCHES = 2;
const int ACTION_TYPE_HELLO         = 3;
const int ACTION_TYPE_BYE           = 4;

const int MESSAGE_ID_LEN         = 80;
const int RELATES_TO_LEN         = 80;
const int ENDPOINT_REFERENCE_LEN = 256;
const int TYPES_LEN              = 256;
const int SCOPES_LEN             = 256;
const int X_ADDRS_LEN            = 256;
const int METADATA_VERSION_LEN   = 80;
const int TO_LEN                 = 80;

struct MSG_DATA
{
   int action_type;
   char header_message_id[MESSAGE_ID_LEN];
   char header_relates_to[RELATES_TO_LEN];
   
   char body_endpoint_reference[ENDPOINT_REFERENCE_LEN];
   char body_types[TYPES_LEN];
   char body_scopes[SCOPES_LEN];
   char body_x_addrs[X_ADDRS_LEN];
   char body_metadata_version[METADATA_VERSION_LEN];
};

class MH_DiscoveryParser
{
private:
    static bool RegisterNamespaces(xmlXPathContextPtr xpath_ctx);

    static bool AnalyzeHello(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg);
    static bool AnalyzeBye(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg);
    static bool AnalyzeProbe(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg);
    static bool AnalyzeProbeMatches(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg);
    
    static bool ParseAction(const char* name, xmlXPathContextPtr xpath_ctx, int* action_type);
    static bool ParseContent(const char* name, xmlXPathContextPtr xpath_ctx, char* val, int len);
    static bool ParseType(const char* name, xmlXPathContextPtr xpath_ctx, char* val, int len);
    static bool ParseAtrr(const char* name, const char* atr_name, xmlXPathContextPtr xpath_ctx, char* val, int len);
    static xmlNodePtr ParseNode(xmlNodeSetPtr nodes);

    static bool ConvertAction(const char* action, int* action_type);

    static void SafeStrCopy(char* dst, int dst_len, const char* src, int src_len = -1);    
    
public:
	static bool Parser(const char* document, int len, MSG_DATA* msg_data);

	static bool CheckTypes(const char* types, const char* type_ns, const char* type);
};

#endif
