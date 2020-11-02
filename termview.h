/* -*- c++ -*- */
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <vector>
#include <memory>
#include "split.h"
class LevelSelector : public wxChoice {
	atque::Resources* rsrc;
public:
	LevelSelector(wxWindow* parent,atque::Resources* rsrc, const std::vector<wxString>& levels);
	DECLARE_EVENT_TABLE();
public:
	void OnSelect(wxCommandEvent &event);
	const atque::Levels* selected();
};

class TerminalGroupSelector : public wxSpinCtrl {
public:
	TerminalGroupSelector(wxWindow* parent);
	void update(int max);
	void OnSelect(wxSpinEvent &event);
	const std::array<std::vector<atque::TermPage>, 3>* selected();
	DECLARE_EVENT_TABLE();
};
class TerminalGrpRadio : public wxRadioBox {
public:
	TerminalGrpRadio(wxWindow* parent);
	void update(const std::array<std::vector<atque::TermPage>, 3>& data);
	void OnSelect(wxCommandEvent &event);
	const std::vector<atque::TermPage>* selected();

	DECLARE_EVENT_TABLE();
};
class TerminalRubiconChkbox : public wxCheckBox {
public:
	TerminalRubiconChkbox(wxWindow* parent)
		:wxCheckBox(parent, 2000, "use Rubicon colors") {}
	void onClick(wxCommandEvent& event);
	DECLARE_EVENT_TABLE();
};

class TerminalPageSlider : public wxSlider {
public:
	 TerminalPageSlider (wxWindow* parent)
		 :wxSlider(parent, 2500, 0, 0, 0, wxDefaultPosition, wxSize(640, 20)) {}
	const atque::TermPage* selected();
	void onScroll(wxScrollEvent& event);
	DECLARE_EVENT_TABLE();
};

class TerminalViewPanel : public wxPanel {
public:
	TerminalViewPanel(wxWindow* parent);
	void update();
	void onPaint(wxPaintEvent& event);
	DECLARE_EVENT_TABLE();
};
class TermView: public wxFrame {
public:
	// read data
	atque::Resources* rsrc;
	LevelSelector* ls;
	TerminalGroupSelector* itemSelector;
	TerminalGrpRadio* termGrpRadio;
	TerminalRubiconChkbox* rubiconCheckbox;
	TerminalPageSlider* pageBar;
	TerminalViewPanel* tPanel;
	TermView(atque::Resources* rsrc, const std::vector<wxString>& levels);
	~TermView();
};
