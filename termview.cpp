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

void  TerminalRubiconChkbox::onClick(wxCommandEvent &event) {
	TermView* parent = static_cast<TermView*>(GetParent());
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

static wxRichTextCtrl* draw_strings(wxWindow* parent, int width, int x, int y,
									const std::vector<atque::TermRichText>& line,
									wxTextAttrAlignment alignment = wxTEXT_ALIGNMENT_LEFT
	) {
	wxRichTextCtrl* ctrl = new wxRichTextCtrl(parent, 5500, wxEmptyString,
											  wxPoint(x, y),
											  wxSize(width, 266),
											  wxVSCROLL | wxRE_MULTILINE | wxBORDER_NONE | wxRE_READONLY );
	TermView* root = static_cast<TermView*>(parent->GetParent());
	ctrl->SetBackgroundColour( *wxBLACK );
	ctrl->BeginAlignment( alignment );
	ctrl->BeginSuppressUndo();
	wxFont font(10, wxFONTFAMILY_MODERN , wxFONTSTYLE_NORMAL,
				wxFONTWEIGHT_NORMAL, false,  wxEmptyString,  wxFONTENCODING_UTF8 );
	ctrl->SetFont( font);
	for( const auto& text : line ) {
		if( text.text == "" ) {
			continue;
		}
		if( text.text == "\n" ) {
			ctrl->Newline();
			continue;
		}
		if( text.b ) {
			ctrl->BeginBold();
		}
		if( text.i ) {
			ctrl->BeginItalic();
		}
		if( text.u ) {
			ctrl->BeginUnderline();
		}
		if( root->rubiconCheckbox->GetValue() ) {
			ctrl->BeginTextColour(  rubiconTermColor[text.color] );
		} else {
			ctrl->BeginTextColour(  termColor[text.color] );
		}
		ctrl->WriteText( text.text );
		ctrl->EndTextColour();
		if( text.u ) {
			ctrl->EndUnderline();
		}
		if( text.i ) {
			ctrl->EndItalic();
		}
		if( text.b ) {
			ctrl->EndBold();
		}
	}
	ctrl->EndAlignment();
	ctrl->EndSuppressUndo();
	return ctrl;
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
		
		auto textMap = draw_strings( this, 640, 0, yp, toDraw->line, wxTEXT_ALIGNMENT_CENTER );
	}
	break;
	case marathon::TerminalGrouping::kPict : {
		auto pict = rsrc->picts[ toDraw->permutation ];
		int x = pict->GetWidth();
		if( toDraw->flags & marathon::TerminalGrouping::kCenterObject ) {			
			new wxStaticBitmap(this, 5000, *pict, wxPoint(72, 27));
		} else if( toDraw->flags & marathon::TerminalGrouping::kDrawObjectOnRight ) {
			draw_strings(this, 307, 9, 27, toDraw->line );
			new wxStaticBitmap(this, 5000, *pict, wxPoint(324, 27));			
		} else {
			new wxStaticBitmap(this, 5000, *pict, wxPoint(9, 27));
			draw_strings(this, 307, 324, 27, toDraw->line );
		}
		
	}
		break;
	case marathon::TerminalGrouping::kCheckpoint : {
		char buf[128];
		snprintf(buf, 128, "CHECKPOINT #%d", toDraw->permutation );
		if( toDraw->flags & marathon::TerminalGrouping::kCenterObject ) {			
			new wxStaticText(this, 2500, buf, wxPoint( 27, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		} else if( toDraw->flags & marathon::TerminalGrouping::kDrawObjectOnRight ) {
			draw_strings(this, 307, 9, 27, toDraw->line );
			new wxStaticText(this, 2500, buf, wxPoint( 324, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		} else {
			new wxStaticText(this, 2500, buf, wxPoint( 9, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
			draw_strings(this, 307, 324, 27, toDraw->line );
		}
		
	}
		break;
	case marathon::TerminalGrouping::kInformation : 
		draw_strings(this, 614, 27, 27, toDraw->line );
		break;
	case marathon::TerminalGrouping::kSound : {
		char buf[128];
		snprintf(buf, 128, "SOUND #%d", toDraw->permutation );
		draw_strings(this, 614, 27, 47, toDraw->line );
		break;
	}
	case marathon::TerminalGrouping::kIntralevelTeleport : {
		char buf[128];
		snprintf(buf, 128, "TELEPORT TO polygon %d", toDraw->permutation );
		new wxStaticText(this, 2500, buf, wxPoint( 27, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		break;
	}
	case marathon::TerminalGrouping::kInterlevelTeleport : {
		std::string name = parent->rsrc->levels[ toDraw->permutation ].name;
		name = "TELEPORT TO " + name;
		new wxStaticText(this, 2500, name, wxPoint( 27, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		break;
	}
	case marathon::TerminalGrouping::kStatic : {
		char buf[128];
		snprintf(buf, 128, "STATIC EFFECT in %d", toDraw->permutation );
		new wxStaticText(this, 2500, buf, wxPoint( 27, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		break;
	}
	case marathon::TerminalGrouping::kTag: {
		char buf[64];
		snprintf(buf, 64, "TAG %d", toDraw->permutation );
		new wxStaticText(this, 2500, buf, wxPoint( 27, 27 ), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		break;
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

BEGIN_EVENT_TABLE( TerminalRubiconChkbox, wxCheckBox)
EVT_CHECKBOX(2000, TerminalRubiconChkbox::onClick)
END_EVENT_TABLE();
