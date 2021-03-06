/*
 *
 *  Copyright (C) 2011-2012 Ghareeb Saad El-Deen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to Amr Thabet
 *  amr.thabet[at]student.alx.edu.eg
 *
 */

#include "StdAfx.h"

#include "SRDF.h"
#include <iostream>

using namespace std;
using namespace Security::Elements::String;
using namespace Security::Targets;
using namespace Security::Libraries::Malware::Enumeration;
using namespace Security::Targets::Memory;
using namespace Security::Libraries::Malware::Static;
int callback(RULE* rule, void* data);

TAG* specified_tags_list = NULL;

typedef struct _IDENTIFIER
{
	char*			name;
	struct _IDENTIFIER*	next;
	
} IDENTIFIER;



IDENTIFIER* specified_rules_list = NULL;

cYaraScanner::cYaraScanner(void)
{
	yr_init();
	Results = new cList(sizeof(YARA_RESULT));
	YContext = yr_create_context();
}

cYaraScanner::~cYaraScanner(void)
{
	for(DWORD i=0;i<Results->GetNumberOfItems();i++)
	{
		((YARA_RESULT*)(Results->GetItem(i)))->Matches->~cList();
	}
	delete Results;
	yr_destroy_context( YContext);
}

char* cYaraScanner::GetLastError()
{
	char* msg = (char*)malloc(1000);
	memset(msg,0,1000);
	yr_get_error_message(YContext,msg,1000);
	return msg;
}
int cYaraScanner::AddRule(cString rule)
{
	return yr_compile_string(rule.GetChar(),YContext);
}

void cYaraScanner::FreeResults()
{
	for(DWORD i=0;i<Results->GetNumberOfItems();i++)
	{
		((YARA_RESULT*)(Results->GetItem(i)))->Matches->~cList();
	}
	delete Results;
	Results=new cList(sizeof(YARA_RESULT));
}

cList* cYaraScanner::Scan(unsigned char* buffer,DWORD buffer_size)
{
	Results = new cList(sizeof(YARA_RESULT));
	if (yr_scan_mem( buffer, buffer_size, YContext,callback, this) != 0)cout << this->GetLastError() << "\n";
	
    return Results;
}

cList* cYaraScanner::Scan(cProcess* Process)
{
	FreeResults();
	yr_scan_proc(Process->ProcessId, YContext,callback, this);
    return Results;
}

//strings = {XX XX XX XX} or *SFEGEGE*
char* cYaraScanner::CreateRule(cString name,cList strings,cString condition)
{

//"rule silent_banker : banker{ strings:$a = {6A 40}$b={8D 4D}$c=\"geb\"condition:$a or $b or $c}

	char id[2]={'a',0};
	
	cString SStrings = "";
	for(DWORD i=0;i<strings.GetNumberOfItems();i++)
	{
		int* x = (int*)strings.GetItem(i);
		char * str = ((cString*)strings.GetItem(i))->GetChar();
		if(str[0] == '{')
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=";
			SStrings+=str;
		}
		else 
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=\"";
			SStrings+=str;
			SStrings+="\"";

		}

		id[0]++;
	}
	int size = strlen(name) + strlen(SStrings) + strlen(condition) + 100;
    char* buffer = (char*)malloc(size);
	memset(buffer,0,size);
	_snprintf(buffer,size-1,"rule %s { strings:%s condition:%s of them }",name.GetChar(),SStrings.GetChar(),condition.GetChar());

	return buffer;

}

char* cYaraScanner::CreateRule(cString name,cList strings,int condition)
{
	cString conditions;

	if (condition == RULE_ALL_OF_THEM)
		conditions=" all of them";
	else
		conditions=" 1 of them";

char id[2]={'a',0};
	
	cString SStrings="";
	for(DWORD i=0;i<strings.GetNumberOfItems();i++)
	{
		int* x= (int*)strings.GetItem(i);
		char * str=((cString*)strings.GetItem(i))->GetChar();
		if(str[0]=='{')
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=";
			SStrings+=str;
		}
		else 
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=\"";
			SStrings+=str;
			SStrings+="\"";
		}

		id[0]++;
	}

	int size = strlen(name) + strlen(SStrings) + strlen(conditions) + 100;
    char* buffer = (char*)malloc(size);
	memset(buffer,0,size);
	_snprintf(buffer,size-1,"rule %s{ strings:%s condition:%s}",name.GetChar(),SStrings.GetChar(),conditions.GetChar());
	return buffer;
}

//For searching for bytes ... write {XX XX XX XX} to search .. although .. it will search for it as a string
char* cYaraScanner::CreateRule(cString name,cString wildcard)
{
	cString conditions = " all of them";
	char id[2]={'a',0};
	
	cString SStrings="";
	;
		char * str = wildcard.GetChar();
		if(str[0]=='{')
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=";
			SStrings+=str;
		}
		else 
		{
			SStrings+="$";
			SStrings+=(char*)id;
			SStrings+="=\"";
			SStrings+=str;
			SStrings+="\"";

		}

		id[0]++;
	

		int size = strlen(name) + strlen(SStrings.GetChar()) + strlen(conditions.GetChar()) + 100;
    char* buffer=(char*)malloc(size);
	memset(buffer,0,size);
	_snprintf(buffer,size-1,"rule %s { strings: %s condition:%s}",name.GetChar(),SStrings.GetChar(),conditions.GetChar());

	return buffer;



}


int callback(RULE* rule, void* data)
{
	cYaraScanner* x = (cYaraScanner*)data;
	return x->ScannerCallback(rule);
}

int cYaraScanner::ScannerCallback(RULE* rule)
{
	YARA_RESULT TResult;
	cYaraScanner* data = this;
    int rule_match;
	int show = TRUE;
	
    rule_match = (rule->flags & RULE_FLAGS_MATCH);

	if (rule->flags & RULE_FLAGS_MATCH)
	{
		YSTRING* gh=rule->string_list_head;
		TResult.RuleIdentifier=rule->identifier;
		TResult.Matches=new cList(sizeof(MSTRING));
		if(gh->matches_head!=NULL)
		{
			while(gh != NULL)
			{
				
				MATCH* match = gh->matches_head;
				while(match != NULL)
				{
					MSTRING* ms=new MSTRING;
					ms->identifier=gh->identifier;
					ms->offset=match->offset;
					ms->data=match->data;
					ms->string=gh->string;
					ms->length=gh->length;
					ms->mlength=match->length;
					TResult.Matches->AddItem((char*)ms);
					match = match->next;
				}
				gh=gh->next;
			}
		}
		Results->AddItem((char*)&TResult);

	}
	
    return CALLBACK_CONTINUE;
}

