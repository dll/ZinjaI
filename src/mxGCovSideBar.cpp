#include "mxGCovSideBar.h"
#include <wx/dcclient.h>
#include "mxSource.h"
#include "mxMainWindow.h"
#include "mxUtils.h"
#include "mxColoursEditor.h"
#include "mxOSD.h"

BEGIN_EVENT_TABLE(mxGCovSideBar, wxWindow)
	EVT_PAINT  (mxGCovSideBar::OnPaint)
END_EVENT_TABLE()

mxGCovSideBar::mxGCovSideBar(wxWindow *parent):wxWindow(parent,wxID_ANY) {
	SetBackgroundColour(*wxWHITE); hits=NULL; SetSize(80,60); should_refresh=NULL;
}

void mxGCovSideBar::OnPaint(wxPaintEvent &event) {
	wxPaintDC dc(this);
	PrepareDC(dc);
	dc.Clear();
	dc.SetPen(wxPen(wxColour(255,0,0)));
	if (!main_window) return;
	mxSource* src=main_window->GetCurrentSource();
	if (!hits || !src) return;
	int cl=src->GetCurrentLine();
	int l0=src->GetFirstVisibleLine();
	int lh=src->TextHeight(l0);
	int sx,sy; src->GetScreenPosition(&sx,&sy);
	int sw,sh; src->GetSize(&sw,&sh);
	int mx,my; GetScreenPosition(&mx,&my);
	int mw,mh; GetSize(&mw,&mh);
	int y=sy-my, l=l0, m2=hits_max/2+1; if (!m2) m2++;
	while (y<sy+sh && l<line_count) {
		if (l==cl) {
			dc.SetBrush(wxBrush(ctheme->CURRENT_LINE));
			dc.SetPen(wxPen(ctheme->CURRENT_LINE));
			dc.DrawRectangle(0,y,mw,lh);
		}
		int h=hits[l++];
		if (h>0) {
			double hp=h;
			int r=hp>=m2?255:(hp)*255/m2;
			int g=hp>=m2?(128-(hp-m2)*128/m2):128;
			wxString text;
			if (h==1) { text="#####"; r=g=0; }
			else if (h>=0) text<<h-1;
			dc.SetTextForeground(wxColour(r,g,0));
			int tw,th;
			GetTextExtent(text,&tw,&th);
			dc.DrawText(text,mw-5-tw,y);
		}
		y+=lh;
	}
}

/// @return false si no pudo ejecutar gcov porque hay otra cosa ejecutandose
bool mxGCovSideBar::ShouldLoadData(mxSource *src) {
	if (!src->sin_titulo && src->source_filename.GetFullPath()==last_path) return false;
	last_path=src->source_filename.GetFullPath();
	return true;
}

void mxGCovSideBar::Refresh (mxSource *src) {
	static int fvl=-1, cl=-1; static mxSource *lsrc=NULL;	
	if (lsrc!=src || src->GetCurrentLine()!=cl || src->GetFirstVisibleLine()!=fvl) {
		lsrc=src;
		cl=src->GetCurrentLine();
		fvl=src->GetFirstVisibleLine();
		if (ShouldLoadData(src)) {
			class ReloadGCovAction:public mxMainWindow::AfterEventsAction {
				public: void Do() { main_window->gcov_sidebar->LoadData(); }
			};
			main_window->CallAfterEvents(new ReloadGCovAction);
		} else wxWindow::Refresh();
	}
}

void mxGCovSideBar::LoadData () {
	if (hits) delete hits; hits=NULL;
	mxSource *src=main_window->GetCurrentSource();
	if (!src) return;
	
	mxOSD osd(main_window,"Generando y leyendo informaci�n de cobertura (gcov)");
	
	wxFileName binary= src->GetBinaryFileName();
	wxFileName fname= binary.GetFullPath().BeforeLast('.')+"."+src->source_filename.GetExt()+".gcov";
	if (binary.FileExists() && (!fname.FileExists() || fname.GetModificationTime()<binary.GetModificationTime())) { 
		wxString command="gcov "; command<<utils->Quotize(binary.GetName());
		utils->Execute(binary.GetPath(),command,wxEXEC_SYNC);
	}
	
	wxTextFile fil(fname.GetFullPath());
	if (!fil.Exists()) return; else fil.Open();
	if (!fil.GetLineCount()) return;
	
	// los valores de hits son los nros de gcov+1, para que 0 sea que la linea no existe (para poder inicializar todo con memset), entonces 1 es que existe pero no se ejecuta, y n es que se ejecuta n-1 veces
	hits=new int[line_count=fil.GetLineCount()+1];
	memset(hits,0,sizeof(int)*line_count);
	hits_max=0;
	for ( wxString line = fil.GetFirstLine(); !fil.Eof(); line = fil.GetNextLine() ) {
		long l=0,h;
		if (!line.AfterFirst(':').BeforeFirst(':').ToLong(&l) || l==0 ||l>=line_count) continue;
		line=line.BeforeFirst(':');
		if (line.Last()=='#') hits[l-1]=1;
		else if (line.Last()=='-') hits[l-1]=0;
		else if (line.ToLong(&h)) {
			hits[l-1]=h+1;
			if (h>hits_max) hits_max=h;
		}
	}
	wxWindow::Refresh();
}
