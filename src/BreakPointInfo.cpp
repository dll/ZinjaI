#include "BreakPointInfo.h"
#include "ProjectManager.h"
#include "mxSource.h"

_autolist_initialize_global(BreakPointInfo);
int BreakPointInfo::last_zinjai_id=0;

BreakPointInfo::BreakPointInfo(file_item *_fitem, int _line_number) {
	_autolist_construct_global; _autolist_construct_local(&(_fitem->breaklist));
	fname=DIR_PLUS_FILE(project->path,_fitem->name);
#if defined(_WIN32) || defined(__WIN32__)
	for (unsigned int i=0;i<fname.Len();i++) // corregir las barras en windows para que no sean caracter de escape
		if (fname[i]=='\\') fname[i]='/';
#endif
	line_number=_line_number;
	source=NULL;
	marker_handle=-1;
	enabled=true;
	only_once=false;
	ignore_count=0;
	gdb_status=BPS_UNKNOWN;
	gdb_id=-1;
	zinjai_id=++last_zinjai_id;
}

BreakPointInfo::BreakPointInfo(mxSource *_source, int _line_number) {
	_autolist_construct_global; _autolist_construct_local(_source->breaklist);
	// para la lista global de breakpoints, siempre se inserta al principio
	fname=_source->GetPathForDebugger();
#if defined(_WIN32) || defined(__WIN32__)
	for (unsigned int i=0;i<fname.Len();i++) // corregir las barras en windows para que no sean caracter de escape
		if (fname[i]=='\\') fname[i]='/';
#endif
	line_number=_line_number;
	gdb_id=-1;
	source=_source;
	enabled=true;
	only_once=false;
	ignore_count=0;
	gdb_status=BPS_UNKNOWN;
	marker_handle=-1; SetMarker();
	zinjai_id=++last_zinjai_id;
}

BreakPointInfo::~BreakPointInfo() {
	_autolist_destroy_global; _autolist_destroy_local;
	if (source && marker_handle!=-1) source->MarkerDeleteHandle(marker_handle);
}

/// @brief Change its gdb_status, set its gdb_id, and update the source's marker if needed
void BreakPointInfo::SetStatus(BREAK_POINT_STATUS _gdb_status, int _gdb_id) {
	gdb_id=_gdb_id;
	gdb_status=_gdb_status;
	SetMarker();
}

/// @brief Updates line_number from the source's marker (it changes when user edits the source code)
void BreakPointInfo::UpdateLineNumber() {
	if (!source) return;
	fname=source->GetPathForDebugger();
#if defined(_WIN32) || defined(__WIN32__)
	for (unsigned int i=0;i<fname.Len();i++) // corregir las barras en windows para que no sean caracter de escape
		if (fname[i]=='\\') fname[i]='/';
#endif
	line_number = source->MarkerLineFromHandle(marker_handle);
}

/// @brief Updates its source_pointer, and ads the marker if its previous one was NULL
void BreakPointInfo::SetSource(mxSource *_source) {
	if (!source) { source=_source; marker_handle=-1; SetMarker(); }
	else source=_source;
}

/// @brief returs true if this breakpoint has a valid gdb_id, inspecting its gdb_status
bool BreakPointInfo::IsInGDB() {
	return gdb_status>BPS_GDB_KNOWS_IT;
}

/// @brief return RedStatus
void BreakPointInfo::SetMarker() {
	if (!source) return;
	int new_type=((gdb_status==BPS_UNKNOWN&&enabled)||gdb_status==BPS_SETTED||gdb_status==BPS_ERROR_CONDITION) ? mxSTC_MARK_BREAKPOINT : mxSTC_MARK_BAD_BREAKPOINT;
	if (marker_handle!=-1) {
		if (marker_type==new_type) return;
		else source->MarkerDeleteHandle(marker_handle);
	}
	marker_handle=source->MarkerAdd(line_number, marker_type=new_type);
}

/**
* @brief find a BreakPointInfo that matches a gdb_id or zinjai_id
*
* If use_gdb finds a breakpoint that matches gdb_id, else 
* finds a breakpoint that matches zinjai_id. If there's no match
* returns NULL.
*
* This method searchs first in opened files (main_window->notebook_sources)
* starting by the current source, then searchs in project's files if there
* is a project.
**/
BreakPointInfo *BreakPointInfo::FindFromNumber(int _id, bool use_gdb_id) {
	BreakPointInfo *bps=NULL;
	while ((bps=GetGlobalNext(bps))) 
		if ( (use_gdb_id&&bps->gdb_id==_id) || (!use_gdb_id&&bps->zinjai_id==_id) )
			return bps;
	return NULL;
}