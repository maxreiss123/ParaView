/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMinMax.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMinMax.h"

#include "vtkArrayMap.txx"
#include "vtkClientServerStream.h"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScalarListWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMinMax);
vtkCxxRevisionMacro(vtkPVMinMax, "1.35");

vtkCxxSetObjectMacro(vtkPVMinMax, ArrayMenu, vtkPVArrayMenu);

//----------------------------------------------------------------------------
vtkPVMinMax::vtkPVMinMax()
{
  this->MinFrame = vtkKWWidget::New();
  this->MinFrame->SetParent(this);
  this->MaxFrame = vtkKWWidget::New();
  this->MaxFrame->SetParent(this);
  this->MinLabel = vtkKWLabel::New();
  this->MaxLabel = vtkKWLabel::New();
  this->MinScale = vtkKWScale::New();
  this->MaxScale = vtkKWScale::New();

  this->SetCommand = NULL;

  this->MinHelp = 0;
  this->MaxHelp = 0;

  this->PackVertically = 1;

  this->ShowMinLabel = 1;
  this->ShowMaxLabel = 1;

  this->MinLabelWidth = 18;
  this->MaxLabelWidth = 18;

  this->ArrayMenu = NULL;

  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkPVMinMax::~vtkPVMinMax()
{
  this->MinScale->Delete();
  this->MinScale = NULL;
  this->MaxScale->Delete();
  this->MaxScale = NULL;
  this->MinLabel->Delete();
  this->MinLabel = NULL;
  this->MaxLabel->Delete();
  this->MaxLabel = NULL;
  this->MinFrame->Delete();
  this->MinFrame = NULL;
  this->MaxFrame->Delete();
  this->MaxFrame = NULL;
  this->SetSetCommand(NULL);
  this->SetMinHelp(0);
  this->SetMaxHelp(0);

  this->SetArrayMenu(NULL);
  
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinimumLabel(const char* label)
{
  this->MinLabel->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaximumLabel(const char* label)
{
  this->MaxLabel->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinimumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MinHelp)
    {
    this->SetMinHelp(help);
    }
  if (this->ShowMinLabel)
    {
    this->MinLabel->SetBalloonHelpString(help);
    }
  this->MinScale->SetBalloonHelpString(help);
  
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaximumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MaxHelp)
    {
    this->SetMaxHelp(help);
    }
  if (this->ShowMaxLabel)
    {
    this->MaxLabel->SetBalloonHelpString(help);
    }
  this->MaxScale->SetBalloonHelpString(help);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // For getting the widget in a script.

  const char* label = this->MinLabel->GetLabel();
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->MinFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x -expand t", 
               this->MinFrame->GetWidgetName());
  if (this->PackVertically)
    {
    this->MaxFrame->Create(pvApp, "frame", "");
    this->Script("pack %s -side top -fill x -expand t", 
                 this->MaxFrame->GetWidgetName());
    }
  
  // Now a label
  if ( this->ShowMinLabel )
    {
    this->MinLabel->SetParent(this->MinFrame);
    ostrstream opts;
    opts << "-width " << this->MinLabelWidth << " -justify right" << ends;
    this->MinLabel->Create(pvApp, opts.str());
    opts.rdbuf()->freeze(0);
    this->Script("pack %s -side left -anchor s", 
                 this->MinLabel->GetWidgetName());
    }

  this->MinScale->SetParent(this->MinFrame);
  this->MinScale->Create(this->GetApplication(), "");
  this->MinScale->SetDisplayEntryAndLabelOnTop(0);
  this->MinScale->DisplayEntry();
  this->MinScale->SetRange(-VTK_LARGE_FLOAT, VTK_LARGE_FLOAT);
  this->MinScale->SetCommand(this, "MinValueCallback");
  this->Script("pack %s -side left -fill x -expand t -padx 5", 
               this->MinScale->GetWidgetName());

  if ( this->ShowMaxLabel )
    {
    if (this->PackVertically)
      {
      this->MaxLabel->SetParent(this->MaxFrame);
      }
    else
      {
      this->MaxLabel->SetParent(this->MinFrame);
      }
    ostrstream opts;
    opts << "-width " << this->MaxLabelWidth << " -justify right" << ends;
    this->MaxLabel->Create(pvApp, opts.str());
    opts.rdbuf()->freeze(0);
    this->Script("pack %s -side left -anchor s", 
                 this->MaxLabel->GetWidgetName());
    }

  if (this->PackVertically)
    {
    this->MaxScale->SetParent(this->MaxFrame);
    }
  else
    {
    this->MaxScale->SetParent(this->MinFrame);
    }
  this->MaxScale->Create(this->GetApplication(), "");
  this->MaxScale->SetDisplayEntryAndLabelOnTop(0);
  this->MaxScale->DisplayEntry();
  this->MaxScale->SetRange(-VTK_LARGE_FLOAT, VTK_LARGE_FLOAT);
  this->MaxScale->SetCommand(this, "MaxValueCallback");
  this->Script("pack %s -side left -fill x -expand t -padx 5", 
               this->MaxScale->GetWidgetName());

  this->SetMinimumHelp(this->MinHelp);
  this->SetMaximumHelp(this->MaxHelp);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinValue(float val)
{
  this->MinScale->SetValue(val);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaxValue(float val)
{
  this->MaxScale->SetValue(val);
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVMinMax::SaveInBatchScript(ofstream *file)
{
  *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
        <<  " GetProperty " << this->SetCommand << "] SetElements2 "
        << this->GetMinValue() << " " << this->GetMaxValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Accept()
{
  int modFlag = this->GetModifiedFlag();

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    prop->SetNumberOfElements(2);
    prop->SetElement(0, this->GetMinValue());
    prop->SetElement(1, this->GetMaxValue());
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }

  this->ModifiedFlag = 0;

  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//---------------------------------------------------------------------------
void vtkPVMinMax::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetMaxValue "
        << this->MaxScale->GetValue() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetMinValue "
        << this->MinScale->GetValue() << endl;
}


//----------------------------------------------------------------------------
void vtkPVMinMax::ResetInternal()
{
  if (!this->AcceptCalled)
    {
    this->Update();
    return;
    }
  if ( this->MinScale->IsCreated() )
    {
    vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetSMProperty());
    if (prop)
      {
      this->SetMinValue(prop->GetElement(0));
      this->SetMaxValue(prop->GetElement(1));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVMinMax::Update()
{
  double range[2];
  double oldRange[2];

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;

  vtkSMProperty* prop = this->GetSMProperty();
  vtkSMDoubleRangeDomain* dom = 0;
  if (prop)
    {
    dom = vtkSMDoubleRangeDomain::SafeDownCast(prop->GetDomain("range"));
    }
  if (dom)
    {
    int exists;
    double rg = dom->GetMinimum(0, exists);
    if (exists)
      {
      range[0] = rg;
      }
    rg = dom->GetMaximum(0, exists);
    if (exists)
      {
      range[1] = rg;
      }
    }

  if (range[0] > range[1])
    {
    vtkErrorMacro("Invalid Data Range");
    return;
    }
  
  if (range[0] == range[1])
    {
    // Special case to avoid log(0).
    this->MinScale->SetRange(range);
    this->MaxScale->SetRange(range);

    this->SetMinValue(range[0]);
    this->SetMaxValue(range[1]);
    return;
    }

  // Find the place value resolution.
  int place = (int)(floor(log10((double)(range[1]-range[0])) - 1.5));
  double resolution = pow(10.0, (double)(place));
  // Now find the range at resolution values.
  range[0] = (floor((double)(range[0]) / resolution) * resolution);
  range[1] = (ceil((double)(range[1]) / resolution) * resolution);


  oldRange[1] = this->MinScale->GetRangeMax();
  oldRange[0] = this->MinScale->GetRangeMin();

  // Detect when the array has changed.
  if (oldRange[0] != range[0] || oldRange[1] != range[1])
    {
    this->MinScale->SetResolution(resolution);
    this->MinScale->SetRange(range);

    this->MaxScale->SetResolution(resolution);
    this->MaxScale->SetRange(range);

    this->SetMinValue(range[0]);
    this->SetMaxValue(range[1]);
    }
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetResolution(float res)
{
  this->MinScale->SetResolution(res);
  this->MaxScale->SetResolution(res);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetRange(float min, float max)
{
  this->MinScale->SetRange(min, max);
  this->MaxScale->SetRange(min, max);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::GetRange(float range[2])
{
  this->MinScale->GetRange(range);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::MinValueCallback()
{
  if (this->MinScale->GetValue() > this->MaxScale->GetValue())
    {
    this->MaxScale->SetValue(this->MinScale->GetValue());
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::MaxValueCallback()
{
  if (this->MaxScale->GetValue() < this->MinScale->GetValue())
    {
    this->MinScale->SetValue(this->MaxScale->GetValue());
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
vtkPVMinMax* vtkPVMinMax::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVMinMax::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVMinMax* pvmm = vtkPVMinMax::SafeDownCast(clone);
  if (pvmm)
    {
    if (this->ArrayMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVArrayMenu* am = this->ArrayMenu->ClonePrototype(pvSource, map);
      pvmm->SetArrayMenu(am);
      am->Delete();
      }

    pvmm->SetMinimumLabel(this->MinLabel->GetLabel());
    pvmm->SetMaximumLabel(this->MaxLabel->GetLabel());
    pvmm->SetMinimumHelp(this->MinHelp);
    pvmm->SetMaximumHelp(this->MaxHelp);
    pvmm->SetResolution(this->MinScale->GetResolution());
    float min, max;
    this->MinScale->GetRange(min, max);
    pvmm->SetRange(min, max);
    pvmm->SetSetCommand(this->SetCommand);
    pvmm->SetMinValue(this->GetMinValue());
    pvmm->SetMaxValue(this->GetMaxValue());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVMinMax.");
    }
}

//----------------------------------------------------------------------------
int vtkPVMinMax::ReadXMLAttributes(vtkPVXMLElement* element,
                                   vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the ArrayMenu.
  const char* array_menu = element->GetAttribute("array_menu");
  if(!array_menu)
    {
    vtkErrorMacro("No array_menu attribute.");
    return 0;
    }
  vtkPVXMLElement* ame = element->LookupElement(array_menu);
  if (!ame)
    {
    vtkErrorMacro("Couldn't find ArrayMenu element " << array_menu);
    return 0;
    }
  vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
  vtkPVArrayMenu* amw = vtkPVArrayMenu::SafeDownCast(w);
  if(!amw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get ArrayMenu widget " << array_menu);
    return 0;
    }
  amw->AddDependent(this);
  this->SetArrayMenu(amw);
  amw->Delete();  

  // Setup the MinimumLabel.
  const char* min_label = element->GetAttribute("min_label");
  if(!min_label)
    {
    vtkErrorMacro("No min_label attribute.");
    return 0;
    }
  this->SetMinimumLabel(min_label);
  
  // Setup the MaximumLabel.
  const char* max_label = element->GetAttribute("max_label");
  if(!max_label)
    {
    vtkErrorMacro("No max_label attribute.");
    return 0;
    }
  this->SetMaximumLabel(max_label);
  
  // Setup the MinimumHelp.
  const char* min_help = element->GetAttribute("min_help");
  if(!min_help)
    {
    vtkErrorMacro("No min_help attribute.");
    return 0;
    }
  this->SetMinimumHelp(min_help);
  
  // Setup the MaximumHelp.
  const char* max_help = element->GetAttribute("max_help");
  if(!max_help)
    {
    vtkErrorMacro("No max_help attribute.");
    return 0;
    }
  this->SetMaximumHelp(max_help);
  
  // Setup the SetCommand.
  const char* set_command = element->GetAttribute("set_command");
  if(!set_command)
    {
    vtkErrorMacro("No set_command attribute.");
    return 0;
    }
  this->SetSetCommand(set_command);
  
  return 1;
}

//----------------------------------------------------------------------------
float vtkPVMinMax::GetMinValue() 
{ return this->MinScale->GetValue(); }

//----------------------------------------------------------------------------
float vtkPVMinMax::GetMaxValue() 
{ return this->MaxScale->GetValue(); }

//----------------------------------------------------------------------------
float vtkPVMinMax::GetResolution() 
{ return this->MinScale->GetResolution(); }

//----------------------------------------------------------------------------
void vtkPVMinMax::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVScalarListWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->SetCommand)+1];
    strcpy(cmd, this->SetCommand);
    int numScalars = 2;
    this->Property->SetVTKCommands(1, &cmd, &numScalars);
    float scalars[2];
    scalars[0] = this->MinScale->GetValue();
    scalars[1] = this->MaxScale->GetValue();
    this->Property->SetScalars(2, scalars);
    delete [] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVMinMax::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVMinMax::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ArrayMenu);  
  this->PropagateEnableState(this->MinLabel);
  this->PropagateEnableState(this->MaxLabel);
  this->PropagateEnableState(this->MinScale);
  this->PropagateEnableState(this->MaxScale);
  this->PropagateEnableState(this->MinFrame);
  this->PropagateEnableState(this->MaxFrame);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "SetCommand: " << (this->SetCommand?this->SetCommand:"none") << endl;
  os << "PackVertically: " << this->PackVertically << endl;
  os << "MinScale: " << this->MinScale << endl;
  os << "MaxScale: " << this->MaxScale << endl;
  os << "ShowMinLabel: " << this->ShowMinLabel << endl;
  os << "ShowMaxLabel: " << this->ShowMaxLabel << endl;
  os << "MinLabelWidth: " << this->MinLabelWidth << endl;
  os << "MaxLabelWidth: " << this->MaxLabelWidth << endl;
}
