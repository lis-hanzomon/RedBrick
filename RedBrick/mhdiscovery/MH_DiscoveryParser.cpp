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

#include "stdafx.h"
#include "MH_DiscoveryParser.h"
#include "mhengine/MH_Engine.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <errno.h>

//-------------------------------------------------------------------------------------------------

bool MH_DiscoveryParser::Parser(const char* content, int length, MSG_DATA* msg_data)
{
    if (msg_data == NULL)
    {
	    return false;
    }
    
    memset(msg_data, 0x00, sizeof(msg_data));
    
    xmlDocPtr doc = xmlReadMemory(content, length, "dummy.xml", NULL, 0);
    if (doc == NULL)
    {
	    return false;
    }
    
    xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
    if(xpath_ctx == NULL)
    {
        xmlFreeDoc(doc);
        return false;
    }
    
    if(!RegisterNamespaces(xpath_ctx))
    {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return false;
    }
    
    if (!ParseAction("/s:Envelope/s:Header/a:Action", xpath_ctx, &msg_data->action_type))
    {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return false;
    }
    
    switch(msg_data->action_type)
    {
    case ACTION_TYPE_PROBE:
        if (!AnalyzeProbe(xpath_ctx, msg_data))
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            return false;
        }
        break;
        
    case ACTION_TYPE_PROBE_MATCHES:
        if (!AnalyzeProbeMatches(xpath_ctx, msg_data))
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            return false;
        }
        break;
        
    case ACTION_TYPE_HELLO:
        if (!AnalyzeHello(xpath_ctx, msg_data))
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            return false;
        }
        break;
        
    case ACTION_TYPE_BYE:
        if (!AnalyzeBye(xpath_ctx, msg_data))
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            return false;
        }
        break;

    default:
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return false;
    }

    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(doc);

    return true;
}

bool MH_DiscoveryParser::RegisterNamespaces(xmlXPathContextPtr xpath_ctx)
{
    if (xpath_ctx == NULL)
    {
	    return false;
    }
    
	if(xmlXPathRegisterNs(xpath_ctx,  BAD_CAST "a", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing") != 0)
    {
	    return false;
	}

	if(xmlXPathRegisterNs(xpath_ctx,  BAD_CAST "d", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery") != 0)
    {
	    return false;
	}

	if(xmlXPathRegisterNs(xpath_ctx,  BAD_CAST "s", BAD_CAST "http://www.w3.org/2003/05/soap-envelope") != 0)
    {
	    return false;
	}

    return true;
}

//-------------------------------------------------------------------------------------------------

bool MH_DiscoveryParser::AnalyzeHello(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg_data)
{
    // message id
    if (!ParseContent("/s:Envelope/s:Header/a:MessageID", xpath_ctx, msg_data->header_message_id, MESSAGE_ID_LEN))
    {
        return false;
    }
    
    // to
    char to[TO_LEN];
    memset(to, 0x00, TO_LEN);
    if (!ParseContent("/s:Envelope/s:Header/a:To", xpath_ctx, to, TO_LEN))
    {
        return false;
    }

    if (strcmp(to, "urn:docs-oasis-open-org:ws-dd:ns:discovery:2009:01") != 0)
    {
        return false;
    }

    // address
    ParseContent("/s:Envelope/s:Body/d:Hello/a:EndpointReference/a:Address", xpath_ctx, msg_data->body_endpoint_reference, ENDPOINT_REFERENCE_LEN);

    // types
    ParseType("/s:Envelope/s:Body/d:Hello/d:Types", xpath_ctx, msg_data->body_types, TYPES_LEN);

    // scopes
    ParseContent("/s:Envelope/s:Body/d:Hello/d:Scopes", xpath_ctx, msg_data->body_scopes, SCOPES_LEN);

    // x addrs
    ParseContent("/s:Envelope/s:Body/d:Hello/d:XAddrs", xpath_ctx, msg_data->body_x_addrs, X_ADDRS_LEN);

    // metadata version
    ParseContent("/s:Envelope/s:Body/d:Hello/d:MetadataVersion", xpath_ctx, msg_data->body_metadata_version, METADATA_VERSION_LEN);

    return true;
}

bool MH_DiscoveryParser::AnalyzeBye(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg_data)
{
    // message id
    if (!ParseContent("/s:Envelope/s:Header/a:MessageID", xpath_ctx, msg_data->header_message_id, MESSAGE_ID_LEN))
    {
        return false;
    }

    // to
    char to[TO_LEN];
    memset(to, 0x00, TO_LEN);
    if (!ParseContent("/s:Envelope/s:Header/a:To", xpath_ctx, to, TO_LEN))
    {
        return false;
    }

    if (strcmp(to, "urn:docs-oasis-open-org:ws-dd:ns:discovery:2009:01") != 0)
    {
        return false;
    }

    // address
    ParseContent("/s:Envelope/s:Body/d:Bye/a:EndpointReference/a:Address", xpath_ctx, msg_data->body_endpoint_reference, ENDPOINT_REFERENCE_LEN);

    // types
    ParseType("/s:Envelope/s:Body/d:Bye/d:Types", xpath_ctx, msg_data->body_types, TYPES_LEN);

    // scopes
    ParseContent("/s:Envelope/s:Body/d:Bye/d:Scopes", xpath_ctx, msg_data->body_scopes, SCOPES_LEN);

    // x addrs
    ParseContent("/s:Envelope/s:Body/d:Bye/d:XAddrs", xpath_ctx, msg_data->body_x_addrs, X_ADDRS_LEN);

    // metadata version
    ParseContent("/s:Envelope/s:Body/d:Bye/d:MetadataVersion", xpath_ctx, msg_data->body_metadata_version, METADATA_VERSION_LEN);

    return true;
}

bool MH_DiscoveryParser::AnalyzeProbe(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg_data)
{
    // message id
    if (!ParseContent("/s:Envelope/s:Header/a:MessageID", xpath_ctx, msg_data->header_message_id, MESSAGE_ID_LEN))
    {
        return false;
    }

    // types
    ParseType("/s:Envelope/s:Body/d:Probe/d:Types", xpath_ctx, msg_data->body_types, TYPES_LEN);

    return true;
}

bool MH_DiscoveryParser::AnalyzeProbeMatches(xmlXPathContextPtr xpath_ctx, MSG_DATA* msg_data)
{
    // message id
    if (!ParseContent("/s:Envelope/s:Header/a:MessageID", xpath_ctx, msg_data->header_message_id, MESSAGE_ID_LEN))
    {
        return false;
    }

    // relates to
    if (!ParseContent("/s:Envelope/s:Header/a:RelatesTo", xpath_ctx, msg_data->header_relates_to, RELATES_TO_LEN))
    {
        return false;
    }

    // address
    ParseContent("/s:Envelope/s:Body/d:ProbeMatches/d:ProbeMatch/a:EndpointReference/a:Address", xpath_ctx, msg_data->body_endpoint_reference, ENDPOINT_REFERENCE_LEN);

    // types
    ParseType("/s:Envelope/s:Body/d:ProbeMatches/d:ProbeMatch/d:Types", xpath_ctx, msg_data->body_types, TYPES_LEN);

    // scopes
    ParseContent("/s:Envelope/s:Body/d:ProbeMatches/d:ProbeMatch/d:Scopes", xpath_ctx, msg_data->body_scopes, SCOPES_LEN);

    // x addrs
    ParseContent("/s:Envelope/s:Body/d:ProbeMatches/d:ProbeMatch/d:XAddrs", xpath_ctx, msg_data->body_x_addrs, X_ADDRS_LEN);

    // metadata version
    ParseContent("/s:Envelope/s:Body/d:ProbeMatches/d:ProbeMatch/d:MetadataVersion", xpath_ctx, msg_data->body_metadata_version, METADATA_VERSION_LEN);

    return true;
}

//-------------------------------------------------------------------------------------------------

bool MH_DiscoveryParser::ParseAction(const char* name, xmlXPathContextPtr xpath_ctx, int* action_type)
{
    if (action_type == NULL)
    {
        return false;
    }
    
    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST name, xpath_ctx);
    if(xpath_obj == NULL)
    {
        *action_type = 0;
        return false;
    }

    xmlNodePtr cur = ParseNode(xpath_obj->nodesetval);
    if (cur == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        *action_type = 0;
        return false;
    }

    const char* action = (const char*)xmlNodeGetContent(cur);
    if (action == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        *action_type = 0;
        return false;
    }

    if (!ConvertAction(action, action_type))
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    xmlXPathFreeObject(xpath_obj);

    return true;
}

bool MH_DiscoveryParser::ParseContent(const char* name, xmlXPathContextPtr xpath_ctx, char* val, int len)
{
    if (val == NULL)
    {
        return false;
    }
    
    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST name, xpath_ctx);
    if(xpath_obj == NULL)
    {
        return false;
    }

    xmlNodePtr cur = ParseNode(xpath_obj->nodesetval);
    if (cur == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    const char* cont = (const char*)xmlNodeGetContent(cur);
    if (cont == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    SafeStrCopy(val, len, cont);

    xmlXPathFreeObject(xpath_obj);
    
    return true;
}

bool MH_DiscoveryParser::ParseType(const char* name, xmlXPathContextPtr xpath_ctx, char* val, int len)
{
    if (val == NULL)
    {
        return false;
    }
    
    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST name, xpath_ctx);
    if(xpath_obj == NULL)
    {
        return false;
    }

    xmlNodePtr cur = ParseNode(xpath_obj->nodesetval);
    if (cur == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    const char* cont = (const char*)xmlNodeGetContent(cur);
    if (cont == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    const char* ns_idx = strchr(cont, ':');
	if (ns_idx != NULL)
	{
		xmlNsPtr* ns_list = xmlGetNsList(cur->doc, cur);
		if (ns_list != NULL)
		{
			xmlNsPtr ns = *ns_list;
			while(ns != NULL)
			{
				int len1 = (int)(ns_idx - cont);
				int len2 = strlen((const char*)(ns->prefix));
				if (len1 == len2)
				{
					if(strncmp(cont, (const char *)(ns->prefix), len1) == 0)
					{
		                SafeStrCopy(val, len, (const char*)(ns->href));
		                
		                int len3 = len - strlen(val);
	                    
		                SafeStrCopy(val + strlen(val), len3, cont + len1);
						break;
					}
				}
				
				ns = ns->next;
			}
		}
	}
	else
	{
	    SafeStrCopy(val, len, cont);
	}
    
    xmlXPathFreeObject(xpath_obj);
    
    return true;
}

bool MH_DiscoveryParser::ParseAtrr(const char* name, const char* atr_name, xmlXPathContextPtr xpath_ctx, char* val, int len)
{
    if (val == NULL)
    {
        return false;
    }

    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST name, xpath_ctx);
    if(xpath_obj == NULL)
    {
        return false;
    }

    xmlNodePtr cur = ParseNode(xpath_obj->nodesetval);
    if (cur == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }

    const char* atr = (const char*)xmlGetProp(cur, (const xmlChar*)atr_name);
    if (atr == NULL)
    {
        xmlXPathFreeObject(xpath_obj);
        return false;
    }
    
    SafeStrCopy(val, len, atr);
    
    xmlXPathFreeObject(xpath_obj);
    
    return true;
}

xmlNodePtr MH_DiscoveryParser::ParseNode(xmlNodeSetPtr nodes)
{
    int size = (nodes) ? nodes->nodeNr : 0;
    if (size != 1)
    {
        return NULL;
    }
    
    if (nodes->nodeTab[0] == NULL)
    {
        return NULL;
    }
    
    xmlNodePtr cur = NULL;
	if(nodes->nodeTab[0]->type == XML_NAMESPACE_DECL)
    {
	    xmlNsPtr ns;
	    ns = (xmlNsPtr)nodes->nodeTab[0];
	    cur = (xmlNodePtr)ns->next;
	}
    else if(nodes->nodeTab[0]->type == XML_ELEMENT_NODE)
    {
	    cur = nodes->nodeTab[0];
	}
    else
    {
	    cur = nodes->nodeTab[0];
	}
    
    return cur;
}

// -----------------------------------------------------------------------------

bool MH_DiscoveryParser::ConvertAction(const char* action, int* action_type)
{
    if (strcmp(action, "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe") == 0)
    {
        *action_type = ACTION_TYPE_PROBE;
    }
    else if (strcmp(action, "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches") == 0)
    {
        *action_type = ACTION_TYPE_PROBE_MATCHES;
    }
    else if (strcmp(action, "http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello") == 0)
    {
        *action_type = ACTION_TYPE_HELLO;
    }
    else if (strcmp(action, "http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye") == 0)
    {
        *action_type = ACTION_TYPE_BYE;
    }
    else
    {
        *action_type = 0;
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------

void MH_DiscoveryParser::SafeStrCopy(char* dst, int dst_size, const char* src, int src_len)
{
	if (src_len == -1)
	{
		src_len = strlen(src);
	}
	
	if (dst_size >= src_len + 1)
	{
		strcpy(dst, src);
	}
	else
	{
		memcpy(dst, src, dst_size);
		*(dst + dst_size - 1) = 0x00;
	}
}

// -----------------------------------------------------------------------------

bool MH_DiscoveryParser::CheckTypes(const char* types, const char* type_ns, const char* type)
{
	int len1 = strlen(type_ns);
	int len2 = strlen(type);
	if ((len1 + 1 + len2) != strlen(types))
	{
		return false;
	}
	
    if (strncmp(type_ns, types, len1) != 0 ||
    	*(types + len1) != ':' ||
        strcmp(type, types + len1 + 1) != 0)
	{
		return false;
	}
	
	return true;
}
