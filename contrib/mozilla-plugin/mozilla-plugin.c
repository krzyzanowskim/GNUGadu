/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include <npapi.h>
#include <npupp.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <glib.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>

#include "mozilla-plugin.h"

#define MIME_TYPES_HANDLED  "text/html"
#define MOZ_PLUGIN_NAME         "Instant Messaging Support "
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":tt:"MOZ_PLUGIN_NAME
#define MOZ_PLUGIN_DESCRIPTION  MOZ_PLUGIN_NAME " Plug-in for Mozilla - This is an experimental plugin \
	developed as a part of <a href='http://www.gnugadu.org'>GNU Gadu</a> project.\
	Handle instant messages protocols and do some work with that.\
	<a href='http://www.freedesktop.org/software/dbus'>DBUS</a>."

/* The instance "object" */
typedef struct _PluginInstance {
	char *url;	/* The URL that this instance displays */
	char *mime_type;	/* The mime type that this instance displays */
	int width, height;	/* The size of the display area */
	unsigned long moz_xid;	/* The XID of the window mozilla has for us */
	pid_t child_pid;	/* pid of the spawned viewer */
	int argc;	
	char **args;
	pthread_t thread;
	NPP instance;
} PluginInstance;

void DEBUGM(const char* format, ...) {
	va_list args;
	va_start(args, format);	
	fprintf(stderr, format, args);
	va_end(args);
}


char* NPP_GetMIMEDescription(void)
{
  DEBUGM("plugin: NPP_getMIMEDescription\n");
  return(MIME_TYPES_DESCRIPTION);
}

// general initialization and shutdown
NPError NPP_Initialize()
{
  DEBUGM("plugin: NPP_Initialise\n");
  return NPERR_NO_ERROR;
}


/* The browser calls this to get the plugin name and description */
NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  NPError err = NPERR_NO_ERROR;
  DEBUGM("plugin: NPP_GetValue1\n");
  
//  NPN_GetValue(instance, NPNVDOMElement, NULL);

  
  switch (variable) {
	case NPPVpluginNameString:
	    *((char **)value) = MOZ_PLUGIN_NAME;
	    DEBUGM("fiu\n");
	break;
	case NPPVpluginDescriptionString:
	    *((char **)value) = MOZ_PLUGIN_DESCRIPTION;
	    DEBUGM("ha\n");
	break;
	default:
	    err = NPERR_GENERIC_ERROR;  
	    DEBUGM("oh\n");
  }
  return err;
}


/* This is called to create a new instance of the plugin
 */
NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc,
		char* argn[], char* argv[], NPSavedData* saved) 
{
    PluginInstance* This;
    DEBUGM("plugin: NPP_New\n");
    
    if (instance == NULL) 
    {
	return NPERR_INVALID_INSTANCE_ERROR;
    }
    
    This = (PluginInstance*) instance->pdata;
    
    return NPERR_NO_ERROR;
}

/* This destroys a plugin instance
 */
NPError NPP_Destroy(NPP instance, NPSavedData** save) 
{
    DEBUGM("plugin: NPP_Destroy\n");
    
    if (instance == NULL) 
    {
	return NPERR_INVALID_INSTANCE_ERROR;
    }
    
    NPN_MemFree(instance->pdata);
    instance->pdata = NULL;
    return NPERR_NO_ERROR;
}

/* This is how the browser passes us a stream. It's also used to tell the
 * browser "we don't want it", and "go download it yourself".
 * TODO: Sometime in the future, this should stream the data if the
 * bonobo control supports PersistStream.
 */
NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream,
		      NPBool seekable, uint16 *stype)
{
    /* By setting this we ask mozilla to download the file for us,
       to save it in its cache, and to tell us when it's done */
    /* *stype = NP_ASFILEONLY; */
    
    DEBUGM("plugin: NPP_NewStream\n");
    return NPERR_NO_ERROR;
}

void NPP_Print(NPP instance, NPPrint* printInfo) {
}

NPError NPP_SetWindow(NPP instance, NPWindow* window) 
{
    DEBUGM("plugin: NPP_SetWindow\n");
    return NPERR_NO_ERROR;
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData) {

	DEBUGM("plugin: NPP_URLNotify(%s)\n", url);
}

void NPP_Shutdown(void) {
	
	DEBUGM("plugin: NPP_Shutdown\n");
}


int32 NPP_WriteReady(NPP instance, NPStream *stream) {
	
	DEBUGM("plugin: NPP_WriteReady\n");
	return 0;
}


int32 NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer) {

	DEBUGM("plugin: NPP_Write\n");
	return 0;
}


NPError NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason) {

	DEBUGM("plugin: NPP_DestroyStream\n");
	return NPERR_NO_ERROR;
}

jref NPP_GetJavaClass() {
	return NULL;
}

int16 NPP_HandleEvent(NPP instance, void *event)
{
	DEBUGM("plugin: handle event\n");
	return 0;
}

