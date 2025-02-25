/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/**********************************************************************
 *<
    FILE: VRML_SCRIPT.cpp

    DESCRIPTION:  A VRML Script helper implementation
 
    CREATED BY: Uwe Woessner
  
    HISTORY: created 7 Jan. 2008
 
 *> Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "vrml.h"
#include "bookmark.h"
#include "script.h"

// Parameter block indices
#define PB_LENGTH 0

//------------------------------------------------------

class VRMLScriptertClassDesc : public ClassDesc
{
public:
    int IsPublic() { return 1; }
    void *Create(BOOL loading = FALSE) { return new VRMLScriptObject; }
    const TCHAR *ClassName() { return GetString(IDS_SCRIPT_CLASS); }
    const TCHAR* NonLocalizedClassName() { return _T("VRMLScript"); }
    SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
    Class_ID ClassID() { return Class_ID(VRML_SCRIPT_CLASS_ID1,
                                         VRML_SCRIPT_CLASS_ID2); }
    const TCHAR *Category() { return _T("VRML97"); }
};

static VRMLScriptertClassDesc VRMLScriptertDesc;

ClassDesc *GetScriptDesc() { return &VRMLScriptertDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

ISpinnerControl *VRMLScriptObject::sizeSpin = NULL;

HWND VRMLScriptObject::hRollup = NULL;

INT_PTR CALLBACK
    VRMLScriptRollupDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    VRMLScriptObject *th = (VRMLScriptObject *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    if (!th && message != WM_INITDIALOG)
        return FALSE;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        th = (VRMLScriptObject *)lParam;
        BOOL usingsize = th->GetUseSize();
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)th);
        SetDlgFont(hDlg, th->iObjParams->GetAppHFont());

        th->sizeSpin = GetISpinner(GetDlgItem(hDlg, IDC_SCRIPT_SIZE_SPINNER));
        th->sizeSpin->SetLimits(0, 999999, FALSE);
        th->sizeSpin->SetScale(1.0f);
        th->sizeSpin->SetValue(th->GetSize(), FALSE);
        th->sizeSpin->LinkToEdit(GetDlgItem(hDlg, IDC_SCRIPT_SIZE), EDITTYPE_POS_FLOAT);
        EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_SIZE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_SIZE_SPINNER), TRUE);

        SendMessage(GetDlgItem(hDlg, IDC_SCRIPT_URL), WM_SETTEXT, 0, (LPARAM)th->insURL.data());
        EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_URL), TRUE);

        CheckDlgButton(hDlg, IDC_SCRIPT_BBOX_SIZE, usingsize);
        CheckDlgButton(hDlg, IDC_SCRIPT_BBOX_DEF, !usingsize);
    }
        return TRUE;

    case WM_DESTROY:
        ReleaseISpinner(th->sizeSpin);
        return FALSE;

    case CC_SPINNER_CHANGE:
        switch (LOWORD(wParam))
        {
        case IDC_SCRIPT_SIZE_SPINNER:
            th->SetSize(th->sizeSpin->GetFVal());
            th->iObjParams->RedrawViews(th->iObjParams->GetTime(), REDRAW_INTERACTIVE);
            break;
        }
        return TRUE;

    case CC_SPINNER_BUTTONUP:
        th->iObjParams->RedrawViews(th->iObjParams->GetTime(), REDRAW_END);
        return TRUE;

    case WM_MOUSEACTIVATE:
        th->iObjParams->RealizeParamPanel();
        return FALSE;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        th->iObjParams->RollupMouseMessage(hDlg, message, wParam, lParam);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BOOKMARKS:
            // do bookmarks
            if (GetBookmarkURL(th->iObjParams, &th->insURL, NULL, NULL))
            {
                // get the new URL information;
                SendMessage(GetDlgItem(hDlg, IDC_SCRIPT_URL), WM_SETTEXT, 0,
                            (LPARAM)th->insURL.data());
            }
            break;
        case IDC_SCRIPT_URL:
            switch (HIWORD(wParam))
            {
            case EN_SETFOCUS:
                DisableAccelerators();
                break;
            case EN_KILLFOCUS:
                EnableAccelerators();
                break;
            case EN_CHANGE:
                int len = (int)SendDlgItemMessage(hDlg, IDC_SCRIPT_URL, WM_GETTEXTLENGTH, 0, 0);
                TSTR temp;
                temp.Resize(len + 1);
                SendDlgItemMessage(hDlg, IDC_SCRIPT_URL, WM_GETTEXT, len + 1, (LPARAM)temp.data());
                th->insURL = temp;
                break;
            }
            break;
        case IDC_SCRIPT_BBOX_SIZE:
        case IDC_SCRIPT_BBOX_DEF:
            th->SetUseSize(IsDlgButtonChecked(hDlg, IDC_SCRIPT_BBOX_SIZE));
            break;
        }
        return FALSE;

    default:
        return FALSE;
    }
}

void
VRMLScriptObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
    iObjParams = ip;

    if (!hRollup)
    {
        hRollup = ip->AddRollupPage(
            hInstance,
            MAKEINTRESOURCE(IDD_SCRIPT),
            VRMLScriptRollupDialogProc,
            GetString(IDS_VRML_SCRIPT_TITLE),
            (LPARAM) this);

        SetWindowLongPtr(hRollup, GWLP_USERDATA, (LONG_PTR) this);
        ip->RegisterDlgWnd(hRollup);
    }
    else
    {
        SetWindowLongPtr(hRollup, GWLP_USERDATA, (LONG_PTR) this);

        // Init the dialog to our values.
    }
}

void
VRMLScriptObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
    if (flags & END_EDIT_REMOVEUI)
    {
        if (hRollup)
        {
            ip->UnRegisterDlgWnd(hRollup);
            ip->DeleteRollupPage(hRollup);
            hRollup = NULL;
        }
    }
    else
    {
        if (hRollup)
            SetWindowLongPtr(hRollup, GWLP_USERDATA, 0);
    }

    iObjParams = NULL;
}

VRMLScriptObject::VRMLScriptObject()
    : HelperObject()
{
    radius = 0.0f;
    useSize = TRUE;
    // Initialize the object from the dlg versions
}

VRMLScriptObject::~VRMLScriptObject()
{
}

void
VRMLScriptObject::SetSize(float r)
{
    radius = r;
    NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

IObjParam *VRMLScriptObject::iObjParams;

// This is only called if the object MAKES references to other things.
#if MAX_PRODUCT_VERSION_MAJOR > 16
RefResult VRMLScriptObject::NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget,
                                             PartID &partID, RefMessage message, BOOL propagate)
#else
RefResult VRMLScriptObject::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
                                             PartID &partID, RefMessage message)
#endif
{
    switch (message)
    {
    case REFMSG_CHANGE:
        // UpdateUI(iObjParams->GetTime());
        break;

    case REFMSG_GET_PARAM_DIM:
    {
        GetParamDim *gpd = (GetParamDim *)partID;
        switch (gpd->index)
        {
        case 0:
            gpd->dim = stdWorldDim;
            break;
        }
        return REF_STOP;
    }

    case REFMSG_GET_PARAM_NAME:
    {
        GetParamName *gpn = (GetParamName *)partID;
        switch (gpn->index)
        {
        case 0:
            // gpn->name = TSTR(GetResString(IDS_DB_TAPE_LENGTH));
            break;
        }
        return REF_STOP;
    }
    }
    return (REF_SUCCEED);
}

ObjectState
VRMLScriptObject::Eval(TimeValue time)
{
    return ObjectState(this);
}

Interval
VRMLScriptObject::ObjectValidity(TimeValue time)
{
    Interval ivalid;
    ivalid.SetInfinite();
    // UpdateUI(time);
    return ivalid;
}

void
VRMLScriptObject::GetMat(TimeValue t, INode *inode, ViewExp *vpt, Matrix3 &tm)
{
    tm = inode->GetObjectTM(t);
#ifdef FIXED_SIZE
    float scaleFactor = vpt->GetVPWorldWidth(tm.GetTrans()) / (float)360.0;
    if (scaleFactor != 1.0f)
        tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));
#endif
}

void
VRMLScriptObject::GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
    Matrix3 m = inode->GetObjectTM(t);
    box = mesh.getBoundingBox();
#ifdef FIXED_SIZE
    float scaleFactor = vpt->GetVPWorldWidth(m.GetTrans()) / (float)360.0;
    box.Scale(scaleFactor);
#endif
}

void
VRMLScriptObject::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
    Matrix3 tm;
    BuildMesh(); // 000829  --prs.
    GetMat(t, inode, vpt, tm);

    int nv = mesh.getNumVerts();
    box.Init();
    for (int i = 0; i < nv; i++)
        box += tm * mesh.getVert(i);
}

void
VRMLScriptObject::MakeQuad(int *f, int a, int b, int c, int d, int vab, int vbc, int vcd, int vda)
{
    mesh.faces[*f].setVerts(a, b, c); // back Face
    mesh.faces[*f].setEdgeVisFlags(vab, vbc, 0);
    mesh.faces[(*f)++].setSmGroup(0);

    mesh.faces[*f].setVerts(c, d, a);
    mesh.faces[*f].setEdgeVisFlags(vcd, vda, 0);
    mesh.faces[(*f)++].setSmGroup(0);
}

void
VRMLScriptObject::BuildMesh()
{
    float size;
    size = radius / 40.0f;
#include "scripticon.cpp"
    mesh.buildBoundingBox();
}

int
VRMLScriptObject::Display(TimeValue t, INode *inode, ViewExp *vpt, int flags)
{
    if (radius <= 0.0)
        return 0;
    BuildMesh();
    Matrix3 m;
    GraphicsWindow *gw = vpt->getGW();
    Material *mtl = gw->getMaterial();

    DWORD rlim = gw->getRndLimits();
    gw->setRndLimits(GW_WIREFRAME | GW_EDGES_ONLY | GW_BACKCULL);
    GetMat(t, inode, vpt, m);
    gw->setTransform(m);
    if (inode->Selected())
        gw->setColor(LINE_COLOR, 1.0f, 1.0f, 1.0f);
    else if (!inode->IsFrozen())
        gw->setColor(LINE_COLOR, 0.0f, 1.0f, 0.0f);
    mesh.render(gw, mtl, NULL, COMP_ALL);
    gw->setRndLimits(rlim);
    return (0);
}

int
VRMLScriptObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
    HitRegion hitRegion;
    DWORD savedLimits;
    int res = FALSE;
    Matrix3 m;
    GraphicsWindow *gw = vpt->getGW();
    Material *mtl = gw->getMaterial();
    MakeHitRegion(hitRegion, type, crossing, 4, p);
    gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
    GetMat(t, inode, vpt, m);
    gw->setTransform(m);
    gw->clearHitCode();
    if (mesh.select(gw, mtl, &hitRegion, flags & HIT_ABORTONHIT))
        return TRUE;
    gw->setRndLimits(savedLimits);
    return res;
}

class VRMLScriptCreateCallBack : public CreateMouseCallBack
{
private:
    IPoint2 sp0;
    Point3 p0;
    VRMLScriptObject *VRMLScriptObject_inst;

public:
    int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m,
             Matrix3 &mat);
    void SetObj(VRMLScriptObject *obj) { VRMLScriptObject_inst = obj; }
};

int
VRMLScriptCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat)
{
    Point3 p1, center;

    switch (msg)
    {
    case MOUSE_POINT:
    case MOUSE_MOVE:
        switch (point)
        {
        case 0: // only happens with MOUSE_POINT msg
            sp0 = m;
            p0 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
            mat.SetTrans(p0);
            break;
        case 1:
            mat.IdentityMatrix();
            p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
            mat.SetTrans(p0);
            VRMLScriptObject_inst->radius = Length(p1 - p0);
            if (VRMLScriptObject_inst->sizeSpin)
                VRMLScriptObject_inst->sizeSpin->SetValue(VRMLScriptObject_inst->radius, FALSE);
            if (flags & MOUSE_CTRL)
            {
                float ang = (float)atan2(p1.y - p0.y, p1.x - p0.x);
                mat.PreRotateZ(VRMLScriptObject_inst->iObjParams->SnapAngle(ang));
            }

            if (msg == MOUSE_POINT)
            {
                return (Length(m - sp0) < 3) ? CREATE_ABORT : CREATE_STOP;
            }
            break;
        }
        break;
    case MOUSE_ABORT:
        return CREATE_ABORT;
    }

    return TRUE;
}

// A single instance of the callback object.
static VRMLScriptCreateCallBack VRMLScriptCreateCB;

// This method allows MAX to access and call our proc method to
// handle the user input.
CreateMouseCallBack *
VRMLScriptObject::GetCreateMouseCallBack()
{
    VRMLScriptCreateCB.SetObj(this);
    return (&VRMLScriptCreateCB);
}

// IO
#define VRML_SCRIPT_SIZE_CHUNK 0xacb0
#define VRML_SCRIPT_URL_CHUNK 0xacb1
#define VRML_SCRIPT_BBOX_CHUNK 0xacb2

IOResult
VRMLScriptObject::Save(ISave *isave)
{
    ULONG written;

    isave->BeginChunk(VRML_SCRIPT_SIZE_CHUNK);
    isave->Write(&radius, sizeof(float), &written);
    isave->EndChunk();

    isave->BeginChunk(VRML_SCRIPT_URL_CHUNK);
#ifdef _UNICODE
    isave->WriteWString(insURL.data());
#else
    isave->WriteCString(insURL.data());
#endif
    isave->EndChunk();

    isave->BeginChunk(VRML_SCRIPT_BBOX_CHUNK);
    isave->Write(&useSize, sizeof(int), &written);
    isave->EndChunk();

    return IO_OK;
}

IOResult
VRMLScriptObject::Load(ILoad *iload)
{
    ULONG nread;
    IOResult res;

    while (IO_OK == (res = iload->OpenChunk()))
    {
        switch (iload->CurChunkID())
        {
        case VRML_SCRIPT_SIZE_CHUNK:
            iload->Read(&radius, sizeof(float), &nread);
            break;
        case VRML_SCRIPT_URL_CHUNK:
        {
            TCHAR *n;
#ifdef _UNICODE
            iload->ReadWStringChunk(&n);
#else
            iload->ReadCStringChunk(&n);
#endif
            insURL = n;
            break;
        }
        case VRML_SCRIPT_BBOX_CHUNK:
        {
            iload->Read((int *)&useSize, sizeof(int), &nread);
            break;
        }
        }
        iload->CloseChunk();
        if (res != IO_OK)
            return res;
    }
    return IO_OK;
}

RefTargetHandle
VRMLScriptObject::Clone(RemapDir &remap)
{
    VRMLScriptObject *vi = new VRMLScriptObject();
    vi->radius = radius;
    vi->insURL = insURL;
    vi->useSize = useSize;
    BaseClone(this, vi, remap);
    return vi;
}
