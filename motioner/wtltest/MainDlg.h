// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

typedef gltestp::dstring dstring;


class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler
{
public:
	enum { IDD = IDD_MAINDLG };

	CEdit m_moveDeltaEdit;
	CListBox m_boneList;
	CListBox m_motionList;
	CListBox m_keyframeList;
	CListBox m_poseList;
	std::vector<dstring> m_poseNames;
	CEdit m_currentFrame;
	CScrollBar m_timeSlider;
	CPreviewDlg m_preview;
	CButton m_combinedPose;
	CEdit m_visibility;
	Model *model;
	ysdnm_t *dnm;
	MotionPose *dnmv;
	Motion *motions[16], *&motion;
	int nmotions, curmotion;
	std::string motion_file[16];
	double motion_time[16];
	double motion_amplitude[16];
	double translate_unit;
	int selectbone;
	int copymotion;
	int copykeyframe;

	CMainDlg() : model(NULL), dnm(NULL), dnmv(NULL), m_preview(*this), motion(motions[0]),
		nmotions(0), curmotion(0), translate_unit(.1), selectbone(0),
		copymotion(-1), copykeyframe(-1)
	{
		memset(motions, 0, sizeof motions);
		memset(motion_time, 0, sizeof motion_time);
		for(int i = 0; i < numof(motion_amplitude); i++)
			motion_amplitude[i] = 1.;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDC_PREVIEW, OnPreview)
		COMMAND_ID_HANDLER(IDC_OPENMODEL, OnOpenModel)
		COMMAND_ID_HANDLER(IDC_OPENMOTION, OnOpenMotion)
		COMMAND_ID_HANDLER(IDC_SAVEMOTION, OnSaveMotion)
		COMMAND_ID_HANDLER(IDC_SAVEMOTION2, OnSaveMotion2)
		COMMAND_ID_HANDLER(IDC_ADDKEYFRAME, OnAddKeyframe)
		COMMAND_ID_HANDLER(IDC_INSERTKEYFRAME, OnInsertKeyframe)
		COMMAND_ID_HANDLER(IDC_COPYKEYFRAME, OnCopyKeyframe)
		COMMAND_ID_HANDLER(IDC_PASTEKEYFRAME, OnPasteKeyframe)
		COMMAND_ID_HANDLER(IDC_DELETEBONEPOSE, OnDeleteBonePose)
//		COMMAND_ID_HANDLER(IDC_ADDBONEPOSE, OnAddBonePose)
		COMMAND_ID_HANDLER(IDC_RXP, OnRXP)
		COMMAND_ID_HANDLER(IDC_RXN, OnRXN)
		COMMAND_ID_HANDLER(IDC_RYP, OnRYP)
		COMMAND_ID_HANDLER(IDC_RYN, OnRYN)
		COMMAND_ID_HANDLER(IDC_RZP, OnRZP)
		COMMAND_ID_HANDLER(IDC_RZN, OnRZN)
		COMMAND_ID_HANDLER(IDC_MXP, OnMXP)
		COMMAND_ID_HANDLER(IDC_MXN, OnMXN)
		COMMAND_ID_HANDLER(IDC_MYP, OnMYP)
		COMMAND_ID_HANDLER(IDC_MYN, OnMYN)
		COMMAND_ID_HANDLER(IDC_MZP, OnMZP)
		COMMAND_ID_HANDLER(IDC_MZN, OnMZN)
		COMMAND_ID_HANDLER(IDC_MOVEDELTA, OnMoveDelta)
		COMMAND_ID_HANDLER(IDC_BONELIST, OnBoneList)
		COMMAND_ID_HANDLER(IDC_MOTIONLIST, OnMotionList)
		COMMAND_ID_HANDLER(IDC_KEYFRAMELIST, OnKeyframeList)
		COMMAND_HANDLER(IDC_POSELIST, LBN_SELCHANGE, OnPoseList)
		COMMAND_ID_HANDLER(IDC_COMBINEDPOSE, OnCombinedPose)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_KEYFRAMETIME, EN_CHANGE, OnKeyframeTimeChange)
		COMMAND_HANDLER(IDC_VISIBILITY, EN_CHANGE, OnVisibilityChange)
		COMMAND_HANDLER(IDC_AMPLITUDE, EN_CHANGE, OnAmplitudeChange)
		NOTIFY_HANDLER(IDC_MOTIONLIST, LBN_SELCHANGE, OnSelChangeMotionList)
//		NOTIFY_HANDLER(IDC_TIMESLIDER, SBN_CHANGE, OnTRBNThumbPosChangingTimeslider)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		m_moveDeltaEdit.Attach(GetDlgItem(IDC_MOVEDELTA));
		TCHAR buf[64];
		swprintf(buf, numof(buf), _T("%g"), translate_unit);
		m_moveDeltaEdit.SetWindowText(buf);
//		this->SetDlgItemInt(IDC_EDITMOVEDELTA, 123);

		m_boneList.Attach(GetDlgItem(IDC_BONELIST));
		m_motionList.Attach(GetDlgItem(IDC_MOTIONLIST));
		m_keyframeList.Attach(GetDlgItem(IDC_KEYFRAMELIST));
		m_poseList.Attach(GetDlgItem(IDC_POSELIST));
		m_currentFrame.Attach(GetDlgItem(IDC_CURRENTFRAME));
		m_timeSlider.Attach(GetDlgItem(IDC_TIMESLIDER));
		m_combinedPose.Attach(GetDlgItem(IDC_COMBINEDPOSE));
		m_visibility.Attach(GetDlgItem(IDC_VISIBILITY));

		for(int i = 0; i < numof(motions); i++)
			m_motionList.AddString(_T("(none)"));
		m_motionList.SetCurSel(0);

		m_poseList.SetHorizontalExtent(1000);

		UIAddChildWindowContainer(m_hWnd);

		return TRUE;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_preview.DestroyWindow();
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
		m_timeSlider.GetScrollInfo(&si);
		si.fMask = SIF_POS;
		switch(LOWORD(wParam)) {
		case SB_TOP:
			si.nPos = si.nMin;
			break;
		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;
		case SB_LINEUP:
			if (si.nPos) si.nPos--;
			break;
		case SB_LINEDOWN:
			if (si.nPos < si.nMax - 1) si.nPos++;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_THUMBPOSITION:
			si.nPos = HIWORD(wParam);
			break;
		}
		m_timeSlider.SetScrollInfo(&si);
		TCHAR buf[128];
		swprintf(buf, _T("%d"), m_timeSlider.GetScrollPos());
		m_currentFrame.SetWindowText(buf);
		motion_time[curmotion] = m_timeSlider.GetScrollPos();

		interp();
		UpdateKeyframeList();

		m_preview.Invalidate();
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnPreview(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		openPreview();
		return 0;
	}

	HWND openPreview()
	{
		if(m_preview.IsWindow())
			m_preview.DestroyWindow();

		RECT r = {100, 100, 600, 600};
		HWND ret = m_preview.Create(NULL, r, _T("Preview Window"));
		m_preview.ShowWindow(SW_SHOW);
		return ret;
	}

	bool OpenModel(const char *file)
	{
		SendMessage(m_boneList, LB_RESETCONTENT, 0, 0);
//					nsufs = LoadMQO_Scale(ofn.lpstrFile, &sufs, &names, .1, &bones);
		model = LoadMQOModel(file, 1., NULL);
		if(!model)
			return false;
		dnmv = new MotionPose;
//		dnmv->bones = model->n;
//		dnmv->bonevar = (ysdnm_bone_var *)malloc(model->n * sizeof(*dnmv->bonevar));
		for(int i = 0; i < model->n; i++){
//			printf("bone %d: depth %d, parent %p\n", i, model->bones[i]->depth, model->bones[i]->parent);

			// Indent to indicate depth hierarchy
			dstring dbuf;
			for(int j = 0; j < model->bones[i]->depth; j++)
				dbuf << ' ';
			dbuf << model->bones[i]->name;

			wchar_t wbuf[128];
			mbstowcs(wbuf, dbuf, numof(wbuf));
			m_boneList.AddString(wbuf);
//			char *newname = new char[::strlen(model->bones[i]->name)+1];
//			::strcpy(newname, model->bones[i]->name);
			MotionNode &bv = dnmv->nodes[model->bones[i]->name];
//			bv.name = newname;
			bv.rot = quat_u;
			bv.pos.clear();
			bv.visible = 1.;
		}
		m_preview.Invalidate();
		return true;
	}

	LRESULT OnOpenModel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static char filter[] = ("Metasequoia Object (*.mqo)\0*.mqo\0");
		static char buf[MAX_PATH] = ("a.mqo");
		OPENFILENAMEA ofn = {sizeof(OPENFILENAMEA), m_hWnd, 0, filter,
			NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
		};
		if(GetOpenFileNameA(&ofn)){
			OpenModel(ofn.lpstrFile);
		}
		return 0;
	}

	LRESULT OnOpenMotion(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
		char buf[MAX_PATH] = "motion.mot";
		OPENFILENAMEA ofn = {sizeof(OPENFILENAME), m_hWnd, 0, filter,
			NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, 0/*OFN_DONTADDTORECENT*/,
		};
		if(GetOpenFileNameA(&ofn)){
			m_motionList.SetHorizontalExtent(1000);
			motions[curmotion] = new Motion(ofn.lpstrFile);

			SCROLLINFO si;
			si.cbSize = sizeof si;
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
			si.nMin = 0;
			si.nMax = motion->totalTime();
			si.nPage = 1;
			si.nPos = 0;
			m_timeSlider.SetScrollInfo(&si);
//			m_timeSlider.SetScrollRange(0, motion->totalTime());

			motion_file[curmotion] = buf;

			UpdateMotionList(curmotion);

			UpdateKeyframeList();
			interp();
		}
		return 0;
	}


	LRESULT OnSaveMotion(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
		char buf[MAX_PATH] = "motion.mot";
		OPENFILENAMEA ofn = {sizeof(OPENFILENAMEA), m_hWnd, 0, filter,
			NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
		};
		if(GetSaveFileNameA(&ofn))
			motions[curmotion]->save(ofn.lpstrFile);
		return 0;
	}

	LRESULT OnSaveMotion2(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
		char buf[MAX_PATH] = "motion.mot";
		OPENFILENAMEA ofn = {sizeof(OPENFILENAMEA), m_hWnd, 0, filter,
			NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
		};
		if(GetSaveFileNameA(&ofn))
			motions[curmotion]->save2(ofn.lpstrFile);
		return 0;
	}

	LRESULT OnAddKeyframe(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(!motions[curmotion]){
			motions[curmotion] = new Motion();
			m_motionList.DeleteString(curmotion);
			m_motionList.InsertString(curmotion, _T("(new)"));
			m_motionList.SetCurSel(curmotion);
		}
		motions[curmotion]->addKeyframe(10.);
		UpdateKeyframeList();
		return 0;
	}

	LRESULT OnInsertKeyframe(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int index = m_keyframeList.GetCurSel();
		if(index < 0 || m_keyframeList.GetCount() <= index)
			return 0;
		if(!motions[curmotion]){
			motions[curmotion] = new Motion();
			m_motionList.DeleteString(curmotion);
			m_motionList.InsertString(curmotion, _T("(new)"));
			m_motionList.SetCurSel(curmotion);
		}
		motions[curmotion]->insertKeyframe(10., index);
		UpdateKeyframeList();
		return 0;
	}

	LRESULT OnCopyKeyframe(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int kflindex = m_keyframeList.GetCurSel();
		if(kflindex < 0 || m_keyframeList.GetCount() <= kflindex)
			return 0;
		copykeyframe = kflindex;
		copymotion = curmotion;
		return 0;
	}

	LRESULT OnPasteKeyframe(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int kflindex = m_keyframeList.GetCurSel();
		if(kflindex < 0 || m_keyframeList.GetCount() <= kflindex)
			return 0;
		if(!motions[copymotion])
			return 0;
		Motion::keyframe *kforg = &motions[copymotion]->kfl[copykeyframe];
		Motion::keyframe *kf = &motions[curmotion]->getKeyframe(motion_time[curmotion]);
		if(!kforg || !kf){
			return 0;
		}
//		kf->ysdnm_motion::keyframe::~keyframe();
		*kf = *kforg;
		interp();
		UpdateKeyframeList();
		return 0;
	}

	LRESULT OnDeleteBonePose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int boneid =selectbone;
		int i;
		MotionPose *dnmv = &motions[curmotion]->getKeyframe(motion_time[curmotion]);
		const char *name;
		if(dnm)
			name = 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
		else if(model)
			name = 0 <= selectbone && selectbone < model->n && model->bones[selectbone] ? model->bones[selectbone]->name : NULL;
		if(!name || !dnmv)
			return false;
//		for(ysdnm_var::iterator it = dnmv->bonevar.begin; it != dnmv->bonevar.end(); i++) if(!strcmp(dnmv->bonevar[i].name, name)){
		if(dnmv->nodes.erase(gltestp::dstring(name))){
/*			delete[] dnmv->bonevar[i].name;
			memmove(&dnmv->bonevar[i], &dnmv->bonevar[i+1], (dnmv->bones - i - 1) * sizeof *dnmv->bonevar);
			dnmv->bones--;*/

			interp();
			UpdateKeyframeList();
			return 0;
		}
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: Add validation code 
		CloseDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}

	LRESULT OnBnClickedAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	MotionNode *AddBone(int boneid){
		int i;
		const char *name = m_preview.GetSrfName(selectbone);
		if(!name || !motions[curmotion])
			return NULL;
		MotionPose *dnmv = &motions[curmotion]->getKeyframe(motion_time[curmotion]);
		if(!dnmv){
			dnmv = new MotionPose;
		}
		return dnmv->addNode(name);
	}

	void Rote(MotionNode *aq, const Vec3d &axis, double angle){
		if(!aq)
			return;
		aq->rot = aq->rot.rotate(angle, axis);
	//	quatrotquat(aq, axis.scale(angle), aq);
		interp();
		UpdateKeyframeList();
	}

	LRESULT OnRXP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_100, M_PI * .01);
		return 0;
	}

	LRESULT OnRXN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_100, M_PI * -.01);
		return 0;
	}

	LRESULT OnRYP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_010, M_PI * .01);
		return 0;
	}

	LRESULT OnRYN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_010, M_PI * -.01);
		return 0;
	}

	LRESULT OnRZP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_001, M_PI * .01);
		return 0;
	}

	LRESULT OnRZN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Rote(AddBone(selectbone), vec3_001, M_PI * -.01);
		return 0;
	}

	void Ofs(MotionNode *aq, const Vec3d &dir, double len){
		if(!aq)
			return;
		aq->pos.addin(dir.scale(len));
		interp();
		UpdateKeyframeList();
	}

	LRESULT OnMXP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_100, translate_unit);
		return 0;
	}

	LRESULT OnMXN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_100, -translate_unit);
		return 0;
	}

	LRESULT OnMYP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_010, translate_unit);
		return 0;
	}

	LRESULT OnMYN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_010, -translate_unit);
		return 0;
	}

	LRESULT OnMZP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_001, translate_unit);
		return 0;
	}

	LRESULT OnMZN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Ofs(AddBone(selectbone), vec3_001, -translate_unit);
		return 0;
	}


	LRESULT OnMoveDelta(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(wNotifyCode == EN_CHANGE){
			TCHAR buf[64];
			GetDlgItemText(IDC_MOVEDELTA, buf, numof(buf));
			translate_unit = _wtof(buf);
		}
		return 1;
	}

	LRESULT OnSelChangeBoneList(WPARAM /*wParam*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/)
	{
		int newbone = m_boneList.GetCurSel();
		if(newbone == selectbone || newbone < 0)
			return 0;

		selectbone = newbone;

		return 0;
	}

	LRESULT OnBoneList(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(wNotifyCode == LBN_SELCHANGE)
			return OnSelChangeBoneList(0, NULL, bHandled);
		return 1;
	}

	LRESULT OnSelChangeMotionList(WPARAM /*wParam*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/)
	{
		int newmotion = m_motionList.GetCurSel();
		if(newmotion == curmotion || newmotion < 0)
			return 0;

		curmotion = newmotion;

		TCHAR tbuf[128];
		swprintf(tbuf, numof(tbuf), _T("%g"), motion_amplitude[curmotion]);
		SetDlgItemTextW(IDC_AMPLITUDE, tbuf);

		UpdateTimeSlider();
		UpdateKeyframeList();
		return 0;
	}

	LRESULT OnKeyframeTimeChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		int kflindex = m_keyframeList.GetCurSel();
		if(kflindex < 0)
			return 0;
		TCHAR tbuf[128];
		GetDlgItemText(IDC_KEYFRAMETIME, tbuf, numof(tbuf));
		char cbuf[128];
		wcstombs(cbuf, tbuf, numof(cbuf));
		double f = atof(cbuf);
		if(f != 0.){
			motions[curmotion]->kfl[kflindex].dt = f;
			UpdateTimeSlider();
			UpdateKeyframeList();
		}
		return 0;
	}

	LRESULT OnVisibilityChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(selectbone < 0)
			return 0;
		TCHAR tbuf[128];
		GetDlgItemText(IDC_VISIBILITY, tbuf, numof(tbuf));
		char cbuf[128];
		wcstombs(cbuf, tbuf, numof(cbuf));
		double f = atof(cbuf);
		MotionNode *v = AddBone(selectbone);
		if(v){
			v->visible = f;
			UpdateKeyframeList();
			interp();
		}
		return 0;
	}

	LRESULT OnAmplitudeChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(curmotion < 0)
			return 0;
		TCHAR tbuf[128];
		GetDlgItemText(IDC_AMPLITUDE, tbuf, numof(tbuf));
		char cbuf[128];
		wcstombs(cbuf, tbuf, numof(cbuf));
		double f = atof(cbuf);
		motion_amplitude[curmotion] = f;
		UpdateMotionList(curmotion);
		interp();
		UpdateKeyframeList();
		return 0;
	}

	LRESULT OnMotionList(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(wNotifyCode == LBN_SELCHANGE)
			return OnSelChangeMotionList(0, NULL, bHandled);
		return 1;
	}

	LRESULT OnKeyframeList(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(wNotifyCode == LBN_SELCHANGE){
			if(motions[curmotion]){
				int kflindex = m_keyframeList.GetCurSel();
				motion_time[curmotion] = motions[curmotion]->getTimeOfKeyframe(kflindex);
				UpdateTimeSlider();
				interp();
				UpdateKeyframeList();
				SetDlgItemInt(IDC_KEYFRAMETIME, motions[curmotion]->kfl[kflindex].dt);
			}
			return 0;
		}
		return 1;
	}

	LRESULT OnPoseList(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		MotionPose *pvar = GetPoseListData();
		if(model && pvar){
			int i = m_poseList.GetCurSel();
			if(0 <= i){
				const dstring &name = m_poseNames[i];
				for(int j = 0; j < model->n; j++){
					if(name == model->bones[j]->name){
						m_boneList.SetCurSel(j);
						selectbone = j;
						break;
					}
				}
				MotionPose *v = &motions[curmotion]->getKeyframe(motion_time[curmotion]);
				if(v){
					MotionPose::NodeMap::iterator it = v->nodes.find(name);
					if(it != v->nodes.end()){
						TCHAR tbuf[128];
						swprintf(tbuf, numof(tbuf), _T("%g"), it->second.visible);
						m_visibility.SetWindowTextW(tbuf);
					}
				}
			}
		}
		return 0;
	}

	LRESULT OnCombinedPose(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		UpdateKeyframeList();
		return 1;
	}

	void interp(){
	//	YSDNM_MotionInterpolateFree(dnmv);
/*		ysdnm_motion *tmpmotions[numof(motions)];
		for(int i = 0; i < numof(motions); i++) if(motions[i])
			tmpmotions[i] = new ysdnm_motion(*motions[i]);
		else
			tmpmotions[i] = NULL;*/
		MotionPose *poses = new MotionPose[numof(motions)];
		for(int i = 0; i < numof(motions); i++){
			if(motions[i])
				motions[i]->interpolate(poses[i], motion_time[i]);
			poses[i].amplify(motion_amplitude[i]);
			poses[i].next = i+1 < numof(motions) ? &poses[i+1] : NULL;
		}
		dnmv = poses;
//		dnmv = YSDNM_MotionInterpolateAmp(motions, motion_time, numof(motions), motion_amplitude);
/*		for(int i = 0; i < numof(motions); i++)
			delete tmpmotions[i];*/
	}

	void UpdateKeyframeList(){

		m_keyframeList.ResetContent();
		if(motions[curmotion]) for(int i = 0; i < motions[curmotion]->kfl.size(); i++){
			TCHAR buf[128];
			swprintf(buf, _T("Keyframe %g"), motions[curmotion]->kfl[i].dt);
			m_keyframeList.AddString(buf);
		}
		if(motions[curmotion])
			m_keyframeList.SetCurSel(motions[curmotion]->getKeyframeIndex(motion_time[curmotion]));

		int oldcur = m_poseList.GetCurSel();
		m_poseList.ResetContent();
		m_poseNames.clear();
		MotionPose *pv = GetPoseListData();
		if(pv){
			MotionPose &v = *pv/*motions[curmotion]->getKeyframe(motion_time[curmotion])*/;
			for(MotionPose::iterator it = v.nodes.begin(); it != v.nodes.end(); it++){
				MotionNode &bv = it->second;
				TCHAR bonename[128];
				mbstowcs(bonename, it->first, numof(bonename));
				TCHAR buf[256];
				swprintf(buf, _T("\"%s\" [%g,%g,%g,%g][%g,%g,%g][%g]"), bonename,
					bv.rot[0], bv.rot[1], bv.rot[2], bv.rot[3],
					bv.pos[0], bv.pos[1], bv.pos[2],
					bv.visible);
				m_poseList.AddString(buf);
				m_poseNames.push_back(it->first);
			}
		}
		m_poseList.SetCurSel(oldcur);

		m_preview.Invalidate();
	}

	MotionPose *GetPoseListData(){
		return IsDlgButtonChecked(IDC_COMBINEDPOSE) ? dnmv : motions[curmotion] ? &motions[curmotion]->getKeyframe(motion_time[curmotion]) : NULL;
	}

	void UpdateTimeSlider(){
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
		si.nMin = 0;
		si.nMax = motions[curmotion] ? motions[curmotion]->totalTime() : 0;
		si.nPage = 1;
		si.nPos = motion_time[curmotion];
		m_timeSlider.SetScrollInfo(&si);
		TCHAR buf[128];
		swprintf(buf, _T("%d"), m_timeSlider.GetScrollPos());
		m_currentFrame.SetWindowText(buf);
	}

	void UpdateMotionList(int index){
		int oldcur = m_motionList.GetCurSel();
		m_motionList.DeleteString(index);
		TCHAR tbuf[MAX_PATH];
		if(motions[index]){
			TCHAR wbuf[MAX_PATH];
			mbstowcs(wbuf, motion_file[index].c_str(), numof(wbuf));
			swprintf(tbuf, numof(tbuf), _T("[%g] %s"), motion_amplitude[index], wbuf);
		}
		else
			wcscpy(tbuf, _T("(none)"));
		m_motionList.InsertString(curmotion, tbuf);
		m_motionList.SetCurSel(oldcur);
	}

//	LRESULT OnTRBNThumbPosChangingTimeslider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
};
