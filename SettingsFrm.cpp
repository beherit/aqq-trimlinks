//---------------------------------------------------------------------------
// Copyright (C) 2012-2014 Krzysztof Grochocki
//
// This file is part of TrimLinks
//
// TrimLinks is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// TrimLinks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "SettingsFrm.h"
#include <inifiles.hpp>
#include <XMLDoc.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "sBevel"
#pragma link "sButton"
#pragma link "sCheckBox"
#pragma link "sLabel"
#pragma link "sSkinManager"
#pragma link "sSkinProvider"
#pragma resource "*.dfm"
TSettingsForm *SettingsForm;
//---------------------------------------------------------------------------
__declspec(dllimport)UnicodeString GetPluginUserDir();
__declspec(dllimport)UnicodeString GetThemeSkinDir();
__declspec(dllimport)bool ChkSkinEnabled();
__declspec(dllimport)bool ChkThemeAnimateWindows();
__declspec(dllimport)bool ChkThemeGlowing();
__declspec(dllimport)int GetHUE();
__declspec(dllimport)int GetSaturation();
__declspec(dllimport)void LoadSettings();
__declspec(dllimport)void RefreshList();
__declspec(dllimport)bool ChkYouTubeListItem();
__declspec(dllimport)UnicodeString GetYouTubeTitleListItem();
__declspec(dllimport)void AddToYouTubeExcludeList(UnicodeString ID);
__declspec(dllimport)UnicodeString ConvertToInt(UnicodeString Text);
__declspec(dllimport)UnicodeString EncodeBase64(UnicodeString Str);
//---------------------------------------------------------------------------
__fastcall TSettingsForm::TSettingsForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::WMTransparency(TMessage &Message)
{
  Application->ProcessMessages();
  if(sSkinManager->Active) sSkinProvider->BorderForm->UpdateExBordersPos(true,(int)Message.LParam);
}
//---------------------------------------------------------------------------

//Pobieranie danych z danego URL
UnicodeString __fastcall TSettingsForm::IdHTTPGet(UnicodeString URL)
{
  //Zmienna z danymi
  UnicodeString ResponseText;
  //Proba pobrania danych
  try
  {
	//Wywolanie polaczenia
	ResponseText = IdHTTP->Get(URL);
  }
  //Blad
  catch(const Exception& e)
  {
	//Hack na wywalanie sie IdHTTP
	if(e.Message=="Connection Closed Gracefully.")
	{
	  //Hack
	  IdHTTP->CheckForGracefulDisconnect(false);
	  //Rozlaczenie polaczenia
	  IdHTTP->Disconnect();
	}
	//Inne bledy
	else
	 //Rozlaczenie polaczenia
	 IdHTTP->Disconnect();
	//Zwrot pustych danych
	return "";
  }
  //Pobranie kodu odpowiedzi
  int Response = IdHTTP->ResponseCode;
  //Wszystko ok
  if(Response==200)
   return ResponseText;
  //Inne bledy
  else
   return "";
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::FormCreate(TObject *Sender)
{
  //Wlaczona zaawansowana stylizacja okien
  if(ChkSkinEnabled())
  {
	UnicodeString ThemeSkinDir = GetThemeSkinDir();
	//Plik zaawansowanej stylizacji okien istnieje
	if(FileExists(ThemeSkinDir + "\\\\Skin.asz"))
	{
	  //Dane pliku zaawansowanej stylizacji okien
	  ThemeSkinDir = StringReplace(ThemeSkinDir, "\\\\", "\\", TReplaceFlags() << rfReplaceAll);
	  sSkinManager->SkinDirectory = ThemeSkinDir;
	  sSkinManager->SkinName = "Skin.asz";
	  //Ustawianie animacji AlphaControls
	  if(ChkThemeAnimateWindows()) sSkinManager->AnimEffects->FormShow->Time = 200;
	  else sSkinManager->AnimEffects->FormShow->Time = 0;
	  sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
	  //Zmiana kolorystyki AlphaControls
	  sSkinManager->HueOffset = GetHUE();
	  sSkinManager->Saturation = GetSaturation();
      //Aktywacja skorkowania AlphaControls
	  sSkinManager->Active = true;
	}
	//Brak pliku zaawansowanej stylizacji okien
	else sSkinManager->Active = false;
  }
  //Zaawansowana stylizacja okien wylaczona
  else sSkinManager->Active = false;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::FormShow(TObject *Sender)
{
  //Odczyt ustawien
  aLoadSettings->Execute();
  //Wylaczenie przycisku
  SaveButton->Enabled = false;
  //Ustawienie fokusa na przycisku
  CancelButton->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aLoadSettingsExecute(TObject *Sender)
{
  //Odczyt ustawien
  TIniFile *Ini = new TIniFile(GetPluginUserDir() + "\\\\TrimLinks\\\\Settings.ini");
  MessagesCheckBox->Checked = Ini->ReadBool("Trim","Messages",true);
  StatusCheckBox->Checked  = Ini->ReadBool("Trim","Status",true);
  delete Ini;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aSaveSettingsExecute(TObject *Sender)
{
  //Zapisywanie ustawien
  TIniFile *Ini = new TIniFile(GetPluginUserDir() + "\\\\TrimLinks\\\\Settings.ini");
  Ini->WriteBool("Trim","Messages",MessagesCheckBox->Checked);
  Ini->WriteBool("Trim","Status",StatusCheckBox->Checked);
  delete Ini;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aSaveSettingsWExecute(TObject *Sender)
{
  //Status przyciskow
  SaveButton->Enabled = false;
  CancelButton->Enabled = false;
  OkButton->Enabled = false;
  //Zapisywanie ustawien
  aSaveSettings->Execute();
  //Odczytywanie ustawien w rdzeniu wtyczki
  LoadSettings();
  //Status przyciskow
  CancelButton->Enabled = true;
  OkButton->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aExitExecute(TObject *Sender)
{
  Close();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::OkButtonClick(TObject *Sender)
{
  aSaveSettingsW->Execute();
  Close();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::GetYouTubeTitleThreadRun(TIdThreadComponent *Sender)
{
  //Pobranie itemu z listy ID do przetworzenia
  UnicodeString ID = GetYouTubeTitleListItem();
  //Jest jakis ID do przetworzenia
  if(!ID.IsEmpty())
  {
	//Pobieranie tytulu
	UnicodeString XML = IdHTTPGet("http://gdata.youtube.com/feeds/api/videos/"+ID+"?fields=title");
	//Parsowanie pliku XML
	if(!XML.IsEmpty())
	{
	  _di_IXMLDocument XMLDoc = LoadXMLData(XML);
	  _di_IXMLNode MainNode = XMLDoc->DocumentElement;
	  _di_IXMLNode ChildNode = MainNode->ChildNodes->GetNode(0);
	  UnicodeString Title = ChildNode->GetText();
	  //Zapisywanie tytulu do cache
	  if(!Title.IsEmpty())
	  {
		TIniFile *Ini = new TIniFile(GetPluginUserDir() + "\\\\TrimLinks\\\\Session.ini");
		Ini->WriteString("YouTube",ConvertToInt(ID),EncodeBase64(Title));
		delete Ini;
	  }
	  //Blokowanie wskaznego ID na czas sesji
	  else AddToYouTubeExcludeList(ID);
	}
	//Blokowanie wskaznego ID na czas sesji
	else AddToYouTubeExcludeList(ID);
  }
  //Brak itemow do przetworzenia
  if(!ChkYouTubeListItem())
  {
	//Zatrzymanie watku
	GetYouTubeTitleThread->Stop();
	//Wlaczenie timera
	RefreshTimer->Enabled = true;
  }
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::RefreshTimerTimer(TObject *Sender)
{
  //Wylaczenie timera
  RefreshTimer->Enabled = false;
  //Odswiezenie listy kontaktow
  RefreshList();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aAllowSaveExecute(TObject *Sender)
{
  SaveButton->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::sSkinManagerSysDlgInit(TacSysDlgData DlgData, bool &AllowSkinning)
{
  AllowSkinning = false;
}
//---------------------------------------------------------------------------

