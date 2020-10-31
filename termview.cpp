#include "atque.h"
#include "termview.h"
#include "split.h"
#include "merge.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <wx/wx.h>
#include <wx/richtext/richtextbuffer.h>
#include <wx/richtext/richtextctrl.h>
bool use_rubicon = false;
LevelSelector::	LevelSelector(wxWindow* parent, atque::Resources* rsrc, const std::vector<wxString>& levels)
	:wxChoice( parent, 1, wxDefaultPosition, wxDefaultSize,
			   levels.size(), levels.data()),
	 rsrc(rsrc)
{
}

const atque::Levels* LevelSelector::selected() {
	int levelId = GetSelection();
	if( levelId == wxNOT_FOUND ) {
		return nullptr;
	}
	return &rsrc->levels[ levelId ];
}
TerminalGroupSelector::TerminalGroupSelector(wxWindow* parent)
	:wxSpinCtrl( parent, 100 ) {
}

const std::array<std::vector<atque::TermPage>, 3>* TerminalGroupSelector::selected() {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto p = parent->ls->selected();
	if( ! p ) {
		return nullptr;
	}
	return &p->pages[ GetValue() ];	
}


void TerminalGroupSelector::update(int max) {
	if( max == -1 ) {
		Enable(false);
	} else {
		Enable(true);
		SetRange(0,  max-1);
		SetValue(0);
	}
}
void TerminalGroupSelector::OnSelect(wxSpinEvent &event) {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto p = selected();
	if( p ) {
		parent->termGrpRadio->update(*p);
	}
	parent->tPanel->update();
}

void TerminalGrpRadio::update(const std::array<std::vector<atque::TermPage>, 3>& gp) {
	TermView* parent = static_cast<TermView*>(GetParent());
	Enable(0, ! gp[0].empty() );
	Enable(1, ! gp[1].empty() );
	Enable(2, ! gp[2].empty() );
	int selected = gp[0].empty() ?
		gp[1].empty() ?
		gp[2].empty() ? -1 : 2
		: 1
		: 0;
	if( selected != -1 ) {
		SetSelection(selected);
		parent->pageBar->SetMax( gp[selected].size() - 1 );
	}
}
void LevelSelector::OnSelect(wxCommandEvent &event) {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto p = selected();
	if( ! p || p->pages.empty() ) {
		parent->itemSelector->update(-1);
	} else {
		parent->itemSelector->update( p->pages.size());
		parent->termGrpRadio->update(p->pages[0]);
	}
	parent->tPanel->update();
}

TerminalViewPanel::TerminalViewPanel(wxWindow* parent)
	:wxPanel(parent, 2500, wxDefaultPosition, wxSize(660, 320)) {
	SetBackgroundColour( *wxBLACK );
}

static const wxString termGrpStr[] = { "UNFINISHED", "FINISHED", "FAILURE" };
TerminalGrpRadio::TerminalGrpRadio(wxWindow* parent)
	:wxRadioBox( parent, 550, "Terminal Groups", wxDefaultPosition, wxDefaultSize, 3, termGrpStr,
				 0, wxRA_SPECIFY_ROWS) {}

TermView::TermView(atque::Resources* rsrc, const std::vector<wxString>& levels)
	:wxFrame(nullptr, wxID_ANY, "Terminal view" )
	,rsrc(rsrc)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	tPanel = new TerminalViewPanel(this);
	ls = new LevelSelector(this, rsrc, levels);
	sizer->Add(ls, wxSizerFlags(0).Center());
	wxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
	sizer2->Add(new wxStaticText(this, 10001, "# TERMINAL:"), 1, wxALIGN_LEFT);

	itemSelector = new TerminalGroupSelector(this);
	itemSelector->SetValue(0);
	sizer2->Add(itemSelector);
	termGrpRadio = new TerminalGrpRadio(this);
	sizer2->Add( termGrpRadio );
	rubiconCheckbox = new TerminalRubiconChkbox(this);
	sizer2->Add( rubiconCheckbox );

	sizer->Add(sizer2);
	sizer->Add( tPanel, wxALIGN_CENTER );
	pageBar = new TerminalPageSlider(this);
	sizer->Add(pageBar,  wxALIGN_CENTER );
	SetSizerAndFit( sizer );
}

TermView::~TermView() {
	delete rsrc;
}

const std::vector<atque::TermPage>* TerminalGrpRadio::selected() {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto p = parent->itemSelector->selected();
	if( ! p ) {
		return nullptr;
	}
	return &(*p)[ GetSelection() ];
}

const atque::TermPage* TerminalPageSlider::selected() {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto tg_sel = parent->termGrpRadio->selected();
	if( ! tg_sel ) {
		return nullptr;
	}
	
	return &(*tg_sel)[ GetValue() ];
}
void TerminalGrpRadio::OnSelect(wxCommandEvent &event) {
	TermView* parent = static_cast<TermView*>(GetParent());
	auto p = selected();
	if( p ) {
		parent->pageBar->SetMax( p->size() - 1 );
	}
	parent->tPanel->update();
}

void TerminalPageSlider::onScroll(wxScrollEvent& event) {
	TermView* parent = static_cast<TermView*>(GetParent());
	parent->tPanel->update();
}

int logon_pos[] = { 27, 9, 293, 631 };
static const wxColour termColor[] = {
	wxColour(0, 255, 0),
	wxColour(255, 255, 255),
	wxColour(255, 0, 0),
	wxColour(0, 153, 0),
	wxColour(0, 255, 255),
	wxColour(255, 255, 0),
	wxColour(153, 0, 0),
	wxColour(0, 0, 255),
};
static wxColour rubiconTermColor[] = {
	wxColour(0, 255, 0),
	wxColour(255, 255, 255),
	wxColour(255, 0, 0),
	wxColour(0, 153, 0),
	wxColour(204, 204, 255),
	wxColour(204, 204, 204),
	wxColour(153, 0, 0),
	wxColour(153, 153, 204),
};

static std::pair<int, int> get_draw_size( int width,
										  const std::vector<atque::TermRichText>& line) {
	wxBitmap bmp(64, 64);
	wxMemoryDC dc(bmp);
	int x = 0, y = 0, hh = 0,ww = 0;
	int hn = 0;
	for( const auto& text : line ) {
		wxFont font(12, wxFONTFAMILY_MODERN ,
					text.i ?  wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
					text.b ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
					text.u,  wxEmptyString,  wxFONTENCODING_UTF8 );
		dc.SetFont( font );
		dc.SetTextForeground( termColor[text.color] );
		wxCoord w, h;
		if( text.text == "\n" ) {
			dc.GetTextExtent( "_", &w, &h );
			x = 0;
			y += h-4;
			hn = 0;
			continue;
		}
		dc.GetTextExtent( text.text, &w, &h );
		hn = h;
		if( x + w > width ) {
			x = w;
			y += h-4;
		} else {
			x += w;
		}
		ww = std::max( ww, x );
		hh = y + h;
	}
	return { ww, hh+hn};
}
static wxBitmap* draw_strings(int width,
							  const std::vector<atque::TermRichText>& line) {
	int x = 0, y = 0;
	auto sz = get_draw_size(width, line);
	if( sz.first <= 0 || sz.second <= 0 ) {
		return nullptr;
	}
	wxBitmap* bmp = new wxBitmap(sz.first, sz.second);
	{
		wxMemoryDC dc(*bmp);
		for( const auto& text : line ) {
			wxFont font(10, wxFONTFAMILY_MODERN ,
						text.i ?  wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
						text.b ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
						text.u,  wxEmptyString,  wxFONTENCODING_UTF8 );
			wxSize ph = font.GetPixelSize();
			dc.SetFont( font );
			dc.SetTextForeground( termColor[text.color] );
			wxCoord w, h;
			if( text.text == "\n" ) {
				dc.GetTextExtent( "_", &w, &h );
				x = 0;
				y += h-4;
				continue;
			}		
			dc.GetTextExtent( text.text, &w, &h );
			if( x + w > width ) {
				x = 0;
				y += h-4;
			}
			dc.DrawText( text.text, x, y + ph.GetHeight() - h );
			x += w;
		}
	}
	return bmp;
}
void TerminalViewPanel::update() { 
	DestroyChildren();
	TermView* parent = static_cast<TermView*>(GetParent());
	auto toDraw = parent->pageBar->selected();
	if( ! toDraw ) {
		return;
	}
	auto rsrc = parent->rsrc;	
	switch( toDraw->type ) {
	case marathon::TerminalGrouping::kLogon :
	case marathon::TerminalGrouping::kLogoff :
	{
		auto pict = rsrc->picts[ toDraw->permutation ];
		int x = pict->GetWidth();
		wxStaticBitmap* pimage = new wxStaticBitmap(this, 5000, *pict,
													wxPoint(logon_pos[1] + (logon_pos[3]-logon_pos[1])/2-x/2, logon_pos[0]));
		int yp = logon_pos[0] + pict->GetHeight();
		int init_x = 9 + 320 - pict->GetWidth() / 2;
		
		wxBitmap* textMap = draw_strings( 640, toDraw->line );
		if( textMap ) {
			init_x = 9 + 320 - textMap->GetWidth() / 2;
			wxStaticBitmap* timage = new wxStaticBitmap(this, 5001, *textMap,
														wxPoint(init_x, yp));
		}
		delete textMap;
	}
	break;
	case marathon::TerminalGrouping::kPict : {
		auto pict = rsrc->picts[ toDraw->permutation ];
		int x = pict->GetWidth();
		if( toDraw->flags & marathon::TerminalGrouping::kCenterObject ) {
			
			wxStaticBitmap* pimage = new wxStaticBitmap(this, 5000, *pict,
														wxPoint(72, 27));
		} else {
			wxStaticBitmap* pimage = new wxStaticBitmap(this, 5000, *pict,
														wxPoint(9, 27));
			wxBitmap* page = draw_strings(307, toDraw->line );
			if( page ) {
				wxScrolledCanvas* canvas = new wxScrolledCanvas(this, 999,
																wxPoint(324, 27),
																wxSize(327, 266),
																wxVSCROLL);
				canvas->SetScrollbars(20, 20, 50, 50);
				canvas->SetVirtualSize( 307, page->GetHeight() + 10);
				wxStaticBitmap* pimage2 = new wxStaticBitmap(canvas, 5000, *page);
				canvas->Show();
				delete page;
			}
		}
		
	}
		break;
	case marathon::TerminalGrouping::kInformation : {
		wxBitmap* page = draw_strings(614, toDraw->line );
		if( page ) {
			wxScrolledCanvas* canvas = new wxScrolledCanvas(this, 999,
															wxPoint(27, 27),
															wxSize(624, 266),
															wxVSCROLL);
			canvas->SetScrollbars(20, 20, 50, 50);
			canvas->SetVirtualSize( 614, page->GetHeight() + 10 );
			wxStaticBitmap* pimage2 = new wxStaticBitmap(canvas, 5000, *page);
			canvas->Show();
			delete page;
		}
		
	}

	}
	parent->Refresh();
}
void TerminalViewPanel::onPaint(wxPaintEvent& event) { 
}

BEGIN_EVENT_TABLE(LevelSelector, wxChoice)
EVT_CHOICE(1, LevelSelector::OnSelect)
END_EVENT_TABLE();

BEGIN_EVENT_TABLE(TerminalGroupSelector, wxSpinCtrl)
EVT_SPINCTRL(100, TerminalGroupSelector::OnSelect)
END_EVENT_TABLE();

BEGIN_EVENT_TABLE(TerminalGrpRadio, wxRadioBox)
EVT_RADIOBOX(550, TerminalGrpRadio::OnSelect)
END_EVENT_TABLE();

BEGIN_EVENT_TABLE( TerminalPageSlider, wxSlider)
EVT_SCROLL(TerminalPageSlider::onScroll)
END_EVENT_TABLE();

BEGIN_EVENT_TABLE( TerminalViewPanel, wxPanel)
END_EVENT_TABLE();
