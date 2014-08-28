/******************************************************************************* 
 *  @file      UIIMList.cpp 2014\8\12 9:07:50 $
 *  @author    大佛<dafo@mogujie.com>
 *  @brief     
 ******************************************************************************/

#include "stdafx.h"
#include "Modules/UI/UIIMList.h"
#include "Modules/IUserListModule.h"
#include "TTLogic/ILogic.h"
#include "TTLogic/ITcpClientModule.h"
#include "utility/utilStrCodeAPI.h"
#include "../SessionManager.h"
/******************************************************************************/

// -----------------------------------------------------------------------------
//  UIIMList: Public, Constructor


const TCHAR* const kLogoButtonControlName = _T("logo");
const TCHAR* const kLogoContainerControlName = _T("logo_container");
const TCHAR* const kNickNameControlName = _T("nickname");
const TCHAR* const kDescriptionControlName = _T("description");
const TCHAR* const kOperatorPannelControlName = _T("operation");


IMListItemInfo::IMListItemInfo()
{

}

IMListItemInfo::~IMListItemInfo()
{
}


UIIMList::UIIMList(CPaintManagerUI& paint_manager)
:paint_manager_(paint_manager)
,root_node_(NULL)
,delay_deltaY_(0)
,delay_number_(0)
,delay_left_(0)
,level_expand_image_(_T("<i MainDialog\\list_icon_b.png>"))
,level_collapse_image_(_T("<i MainDialog\\list_icon_a.png>"))
,level_text_start_pos_(0)
,text_padding_(0, 0, 0, 0)
{
	SetItemShowHtml(true);
	SetContextMenuUsed(true);

	root_node_ = new Node;
	root_node_->data().level_ = -1;
	root_node_->data().child_visible_ = true;
	root_node_->data().has_child_ = true;
	root_node_->data().list_elment_ = NULL;
}

// -----------------------------------------------------------------------------
//  UIIMList: Public, Destructor

UIIMList::~UIIMList()
{
	delete root_node_;
	root_node_ = NULL;
	logic::GetLogic()->removeObserver(this);
}

bool UIIMList::Add(CControlUI* pControl)
{
	if (!pControl)
		return false;

	if (_tcsicmp(pControl->GetClass(), _T("ListContainerElementUI")) != 0)
		return false;

	return CListUI::Add(pControl);
}
bool UIIMList::AddAt(CControlUI* pControl, int iIndex)
{
	if (!pControl)
		return false;

	if (_tcsicmp(pControl->GetClass(), _T("ListContainerElementUI")) != 0)
		return false;

	return CListUI::AddAt(pControl, iIndex);
}

bool UIIMList::Remove(CControlUI* pControl)
{
	if (!pControl)
		return false;

	if (_tcsicmp(pControl->GetClass(), _T("ListContainerElementUI")) != 0)
		return false;

	if (reinterpret_cast<Node*>(static_cast<CListContainerElementUI*>(pControl->GetInterface(_T("ListContainerElement")))->GetTag()) == NULL)
		return CListUI::Remove(pControl);
	else
		return RemoveNode(reinterpret_cast<Node*>(static_cast<CListContainerElementUI*>(pControl->GetInterface(_T("ListContainerElement")))->GetTag()));
}

bool UIIMList::RemoveAt(int iIndex)
{
	CControlUI* pControl = GetItemAt(iIndex);
	if (!pControl)
		return false;

	if (_tcsicmp(pControl->GetClass(), _T("ListContainerElementUI")) != 0)
		return false;

	if (reinterpret_cast<Node*>(static_cast<CListContainerElementUI*>(pControl->GetInterface(_T("ListContainerElement")))->GetTag()) == NULL)
		return CListUI::RemoveAt(iIndex);
	else
		return RemoveNode(reinterpret_cast<Node*>(static_cast<CListContainerElementUI*>(pControl->GetInterface(_T("ListContainerElement")))->GetTag()));
}

void UIIMList::RemoveAll()
{
	CListUI::RemoveAll();
	for (int i = 0; i < root_node_->num_children(); ++i)
	{
		Node* child = root_node_->child(i);
		RemoveNode(child);
	}
	delete root_node_;

	root_node_ = new Node;
	root_node_->data().level_ = -1;
	root_node_->data().child_visible_ = true;
	root_node_->data().has_child_ = true;
	root_node_->data().list_elment_ = NULL;
}

void UIIMList::DoEvent(TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND)
	{
		if (m_pParent != NULL)
			m_pParent->DoEvent(event);
		else
			CVerticalLayoutUI::DoEvent(event);
		return;
	}

	if (event.Type == UIEVENT_TIMER && event.wParam == SCROLL_TIMERID)
	{
		if (delay_left_ > 0)
		{
			--delay_left_;
			SIZE sz = GetScrollPos();
			LONG lDeltaY = (LONG)(CalculateDelay((double)delay_left_ / delay_number_) * delay_deltaY_);
			if ((lDeltaY > 0 && sz.cy != 0) || (lDeltaY < 0 && sz.cy != GetScrollRange().cy))
			{
				sz.cy -= lDeltaY;
				SetScrollPos(sz);
				return;
			}
		}
		delay_deltaY_ = 0;
		delay_number_ = 0;
		delay_left_ = 0;
		m_pManager->KillTimer(this, SCROLL_TIMERID);
		return;
	}
	if (event.Type == UIEVENT_SCROLLWHEEL)
	{
		LONG lDeltaY = 0;
		if (delay_number_ > 0)
			lDeltaY = (LONG)(CalculateDelay((double)delay_left_ / delay_number_) * delay_deltaY_);
		switch (LOWORD(event.wParam))
		{
		case SB_LINEUP:
			if (delay_deltaY_ >= 0)
				delay_deltaY_ = lDeltaY + 8;
			else
				delay_deltaY_ = lDeltaY + 12;
			break;
		case SB_LINEDOWN:
			if (delay_deltaY_ <= 0)
				delay_deltaY_ = lDeltaY - 8;
			else
				delay_deltaY_ = lDeltaY - 12;
			break;
		}
		if
			(delay_deltaY_ > 100) delay_deltaY_ = 100;
		else if
			(delay_deltaY_ < -100) delay_deltaY_ = -100;

		delay_number_ = (DWORD)sqrt((double)abs(delay_deltaY_)) * 5;
		delay_left_ = delay_number_;
		m_pManager->SetTimer(this, SCROLL_TIMERID, 50U);
		return;
	}

	CListUI::DoEvent(event);
}

Node* UIIMList::GetRoot()
{
	return root_node_;
}

/******************************************************************************/

static bool OnLogoButtonEvent(void* event) {
	if (((TEventUI*)event)->Type == UIEVENT_BUTTONDOWN) {
		CControlUI* pButton = ((TEventUI*)event)->pSender;
		if (pButton != NULL) {
			CListContainerElementUI* pListElement = (CListContainerElementUI*)(pButton->GetTag());
			if (pListElement != NULL) pListElement->DoEvent(*(TEventUI*)event);
		}
	}
	return true;
}
Node* UIIMList::AddNode(const IMListItemInfo& item, Node* parent, int index)
{
	if (!parent)
		parent = root_node_;

	TCHAR szBuf[MAX_PATH] = { 0 };

	CListContainerElementUI* pListElement = NULL;
	if (!m_dlgBuilder.GetMarkup()->IsValid())
	{
		if (!m_IitemXmlFilePath.IsEmpty())
		{
			pListElement = static_cast<CListContainerElementUI*>(m_dlgBuilder.Create(STRINGorID(m_IitemXmlFilePath), (UINT)0, NULL, &paint_manager_));
		}else
		pListElement = static_cast<CListContainerElementUI*>(m_dlgBuilder.Create(_T("MainDialog\\recentlyListItem.xml"), (UINT)0, NULL, &paint_manager_));
	}
	else
	{
		pListElement = static_cast<CListContainerElementUI*>(m_dlgBuilder.Create((UINT)0, &paint_manager_));
	}
	if (pListElement == NULL)
		return NULL;

	Node* node = new Node;

	node->data().level_ = parent->data().level_ + 1;
	if (item.folder)
		node->data().has_child_ = !item.empty;
	else
		node->data().has_child_ = false;

	node->data().folder_ = item.folder;

	node->data().child_visible_ = (node->data().level_ == 0);
	node->data().sId = item.id;
	node->data().text_ = item.nickName;
	node->data().list_elment_ = pListElement;

	if (!parent->data().child_visible_)
		pListElement->SetVisible(false);

	if (parent != root_node_ && !parent->data().list_elment_->IsVisible())
		pListElement->SetVisible(false);

	CDuiRect rcPadding = text_padding_;
	for (int i = 0; i < node->data().level_; ++i)
	{
		rcPadding.left += level_text_start_pos_;
	}
	pListElement->SetPadding(rcPadding);

	CButtonUI* log_button = static_cast<CButtonUI*>(paint_manager_.FindSubControlByName(pListElement, kLogoButtonControlName));
	if (log_button != NULL)
	{
		if (!item.folder && !item.avatarPath.IsEmpty())
		{
			_stprintf_s(szBuf, MAX_PATH - 1, _T("%s"), item.avatarPath);
			log_button->SetNormalImage(szBuf);
		}
		else
		{
			CContainerUI* logo_container = static_cast<CContainerUI*>(paint_manager_.FindSubControlByName(pListElement, kLogoContainerControlName));
			if (logo_container != NULL)
				logo_container->SetVisible(false);
		}
		log_button->SetTag((UINT_PTR)pListElement);
		log_button->OnEvent += MakeDelegate(&OnLogoButtonEvent);
	}

	CDuiString html_text;
	if (node->data().has_child_)
	{
		if (node->data().child_visible_)
			html_text += level_expand_image_;
		else
			html_text += level_collapse_image_;

		_stprintf_s(szBuf, MAX_PATH - 1, _T("<x %d>"), level_text_start_pos_);
		html_text += szBuf;
	}

	if (item.folder)
	{
		html_text += node->data().text_;
	}
	else
	{
		_stprintf_s(szBuf, MAX_PATH - 1, _T("%s"), item.nickName);
		html_text += szBuf;
	}

	CLabelUI* nick_name = static_cast<CLabelUI*>(paint_manager_.FindSubControlByName(pListElement, kNickNameControlName));
	if (nick_name != NULL)
	{
		if (item.folder)
			nick_name->SetFixedWidth(0);

		nick_name->SetShowHtml(true);
		nick_name->SetText(html_text);
	}

	if (!item.folder && !item.description.IsEmpty())
	{
		CLabelUI* description = static_cast<CLabelUI*>(paint_manager_.FindSubControlByName(pListElement, kDescriptionControlName));
		if (description != NULL)
		{
			_stprintf_s(szBuf, MAX_PATH - 1, _T("<x 20><c #808080>%s</c>"), item.description);
			description->SetShowHtml(true);
			description->SetText(szBuf);
		}
	}

	pListElement->SetFixedHeight(m_IMListItemNormalHeight);
	pListElement->SetTag((UINT_PTR)node);

	if (0 == index)
	{
		if (parent->has_children())
		{
			Node* prev = parent->get_last_child();
			index = prev->data().list_elment_->GetIndex() + 1;
		}
		else
		{
			if (parent == root_node_)
				index = 0;
			else
				index = parent->data().list_elment_->GetIndex() + 1;
		}
	}

	if (!CListUI::AddAt(pListElement, index))
	{
		delete pListElement;
		delete node;
		node = NULL;
	}

	parent->add_child(node);
	return node;
}

bool UIIMList::RemoveNode(Node* node)
{
	if (!node || node == root_node_) return false;

	for (int i = 0; i < node->num_children(); ++i)
	{
		Node* child = node->child(i);
		RemoveNode(child);
	}

	CListUI::Remove(node->data().list_elment_);
	node->parent()->remove_child(node);
	delete node;

	return true;
}

void UIIMList::SetChildVisible(Node* node, bool visible)
{
	if (!node || node == root_node_)
		return;

	if (node->data().child_visible_ == visible)
		return;

	node->data().child_visible_ = visible;

	TCHAR szBuf[MAX_PATH] = { 0 };
	CDuiString html_text;
	if (node->data().has_child_)
	{
		if (node->data().child_visible_)
			html_text += level_expand_image_;
		else
			html_text += level_collapse_image_;

		_stprintf_s(szBuf, MAX_PATH - 1, _T("<x %d>"), level_text_start_pos_);

		html_text += szBuf;
		html_text += node->data().text_;

		CLabelUI* nick_name = static_cast<CLabelUI*>(paint_manager_.FindSubControlByName(node->data().list_elment_, kNickNameControlName));
		if (nick_name != NULL)
		{
			nick_name->SetShowHtml(true);
			nick_name->SetText(html_text);
		}
	}

	if (!node->data().list_elment_->IsVisible())
		return;

	if (!node->has_children())
		return;

	Node* begin = node->child(0);
	Node* end = node->get_last_child();
	for (int i = begin->data().list_elment_->GetIndex(); i <= end->data().list_elment_->GetIndex(); ++i)
	{
		CControlUI* control = GetItemAt(i);
		if (_tcsicmp(control->GetClass(), _T("ListContainerElementUI")) == 0)
		{
			if (visible)
			{
				Node* local_parent = ((Node*)control->GetTag())->parent();

				if (local_parent->data().child_visible_
					&&local_parent->data().list_elment_
					&&local_parent->data().list_elment_->IsVisible())
				{
					control->SetVisible(true);
				}
			}
			else
			{
				control->SetVisible(false);
			}
		}
	}
}

bool UIIMList::CanExpand(Node* node) const
{
	if (!node || node == root_node_) return false;

	return node->data().has_child_;
}

void UIIMList::SetIitemXmlFile(CString IitemXml)
{
	m_IitemXmlFilePath = IitemXml;
}

void UIIMList::SetItemNormalHeight(const int nIMListItemNormalHeight)
{
	m_IMListItemNormalHeight = nIMListItemNormalHeight;
}

Node* UIIMList::GetItemBySId(IN const std::string& sId)
{
	if (!root_node_)
		return 0;
	std::vector<std::string> vecMembers;
	for (int i = 0; i < root_node_->num_children(); ++i)
	{
		Node* child = root_node_->child(i);
		if (child)
		{
			CString strID = child->data().sId;
			if (util::cStringToString(strID) == sId)
			{
				return child;
			}
			if (child->has_children())
			{
				Node*	subRoot = child;
				for (int p = 0; p < subRoot->num_children(); ++p)	//只有两层，多层要用递归
				{
					child = subRoot->child(p);
					CString strID = child->data().sId;
					if (util::cStringToString(strID) == sId)
					{
						return child;
					}
				}
			}
		}
	}
	return 0;
}

BOOL UIIMList::IsExistSId(IN const std::string& sId)
{
	return (GetItemBySId(sId) != 0);
}

void UIIMList::ExpandAll()
{
	for (int i = 0; i < root_node_->num_children(); ++i)
	{
		Node* child = root_node_->child(i);
		if (CanExpand(child))
		{
			SetChildVisible(child, true);
		}
	}
}

std::vector<std::string> UIIMList::GetAllItemsSId()
{
	std::vector<std::string> vecMembers;
	for (int i = 0; i < root_node_->num_children(); ++i)
	{
		Node* child = root_node_->child(i);
		if (child)
		{
			if (child->has_children())
			{
				Node*	subRoot = child;
				for (int p = 0; p < subRoot->num_children(); ++p)	//只有两层，多层要用递归
				{
					child = subRoot->child(p);
					CString strID = child->data().sId;
					vecMembers.push_back(util::cStringToString(strID));
				}
			}
			else
			{
				if (!child->folder())
				{
					CString strID = child->data().sId;
					vecMembers.push_back(util::cStringToString(strID));
				}
			}
		}
	}
	return vecMembers;
}

BOOL UIIMList::AddNodeUnderTheSid(IN CDuiString sId, OUT Node& parent)
{
	for (int i = 0; i < root_node_->num_children(); ++i)
	{
		Node* child = root_node_->child(i);
		if (child && child->folder())
		{
			CDuiString strID = child->data().sId;
			if (strID == sId)
			{
				parent = *child;
				CListContainerElementUI* plistElmen = child->data().list_elment_;
				if (plistElmen)
				{
					CString strParentName = child->data().text_;
					CLabelUI* nick_name = static_cast<CLabelUI*>(plistElmen->FindSubControl(kNickNameControlName));
					if (nick_name != NULL)
					{
						strParentName.Format(_T("%s(%d)"), strParentName, parent.num_children());
						nick_name->SetText(strParentName);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

void UIIMList::OnUserListModuleEvent(UInt16 moduleId, UInt32 keyId, MKO_TUPLE_PARAM mkoParam)
{
	if (module::KEY_USERLIST_DOWNAVATAR_SUCC == keyId)
	{
		//刷新头像
		std::string& sId = std::get<MKO_STRING>(mkoParam);
		_FreshAvatarBySId(sId);
	}
	else if (module::KEY_USERLIST_ALLUSERLINESTATE == keyId)
	{
		//刷新个人在线状态，群不刷新
		std::vector<std::string>vecSIds = GetAllItemsSId();
		for (std::string sId : vecSIds)
		{
			SessionEntity* pSession = SessionEntityManager::getInstance()->getSessionEntityBySId(sId);
			if(pSession && SESSION_USERTYPE == pSession->m_sessionType)
				_FreshAvatarBySId(sId);
		}
	}
	else if (module::KEY_USERLIST_USERLINESTATE == keyId)
	{
		//刷新个人在线状态，群不刷新
		std::string& sId = std::get<MKO_STRING>(mkoParam);
		SessionEntity* pSession = SessionEntityManager::getInstance()->getSessionEntityBySId(sId);
		if(pSession && SESSION_USERTYPE == pSession->m_sessionType)
			_FreshAvatarBySId(sId);
	}
}

void UIIMList::DoInit()
{
	logic::GetLogic()->addObserver(this, MODULE_ID_USERLIST
		, fastdelegate::MakeDelegate(this, &UIIMList::OnUserListModuleEvent));
	logic::GetLogic()->addObserver(this, MODULE_ID_TCPCLIENT
		, fastdelegate::MakeDelegate(this, &UIIMList::OnTcpClientModuleEvent));
}

void UIIMList::_FreshAvatarBySId(const std::string& sId)
{
	Node* pNode = GetItemBySId(sId);
	if (!pNode)
		return;
	CListContainerElementUI* pListElement = pNode->data().list_elment_;
	CButtonUI* btnLogo = static_cast<CButtonUI*>(paint_manager_.FindSubControlByName(pListElement, kLogoButtonControlName));
	PTR_VOID(btnLogo);

	module::UserInfoEntity user;
	if(module::getUserListModule()->getUserInfoBySId(sId, user))
		btnLogo->SetNormalImage(util::stringToCString(user.getAvatarPath()));
}

void UIIMList::OnTcpClientModuleEvent(UInt16 moduleId, UInt32 keyId, MKO_TUPLE_PARAM mkoParam)
{
	if (logic::KEY_TCPCLIENT_STATE == keyId)
	{
		//TCP长连断开处理
		if (logic::TCPCLIENT_STATE_DISCONNECT == logic::getTcpClientModule()->getTcpClientNetState())
		{
			//刷新个人在线状态，群不刷新
			std::vector<std::string>vecSIds = GetAllItemsSId();
			for (std::string sId : vecSIds)
			{
				SessionEntity* pSession = SessionEntityManager::getInstance()->getSessionEntityBySId(sId);
				if (pSession && SESSION_USERTYPE == pSession->m_sessionType)
				{
					_FreshAvatarBySId(sId);
				}
			}
		}
	}
}