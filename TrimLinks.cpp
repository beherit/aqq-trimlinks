//---------------------------------------------------------------------------
// Copyright (C) 2012-2015 Krzysztof Grochocki
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

#include <vcl.h>
#include <windows.h>
#include <inifiles.hpp>
#include <IdHashMessageDigest.hpp>
#include <PluginAPI.h>
#include <LangAPI.hpp>
#pragma hdrstop
#include "SettingsFrm.h"

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------

//Uchwyt-do-formy-ustawien---------------------------------------------------
TSettingsForm *hSettingsForm;
//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
//Sciezka-do-pliku-sesji-----------------------------------------------------
UnicodeString SessionFileDir;
//Gdy-zostalo-uruchomione-zaladowanie-wtyczki--------------------------------
bool LoadExecuted = false;
//Gdy-zostalo-uruchomione-wyladowanie-wtyczki-wraz-z-zamknieciem-komunikatora
bool ForceUnloadExecuted = false;
//Informacja-o-widocznym-oknie-instalowania-dodatku--------------------------
bool FrmInstallAddonExist = false;
//Lista-ID-filmow-YouTube-do-przetworzenia-----------------------------------
TStringList *GetYouTubeTitleList = new TStringList;
//Lista-ID-filmow-YouTube-wykluczonych-na-czas-sesji-------------------------
TStringList *YouTubeExcludeList = new TStringList;
//SETTINGS-------------------------------------------------------------------
bool TrimMessages;
bool TrimStatus = true;
//FORWARD-AQQ-HOOKS----------------------------------------------------------
INT_PTR __stdcall OnAddLine(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnSetHTMLStatus(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Pobieranie sciezki katalogu prywatnego wtyczek
UnicodeString GetPluginUserDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR, 0, 0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki skorki kompozycji
UnicodeString GetThemeSkinDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR, 0, 0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Skin";
}
//---------------------------------------------------------------------------

//Sprawdzanie czy wlaczona jest zaawansowana stylizacja okien
bool ChkSkinEnabled()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString SkinsEnabled = Settings->ReadString("Settings", "UseSkin", "1");
	delete Settings;
	return StrToBool(SkinsEnabled);
}
//---------------------------------------------------------------------------

//Pobieranie ustawien animacji AlphaControls
bool ChkThemeAnimateWindows()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString AnimateWindowsEnabled = Settings->ReadString("Theme", "ThemeAnimateWindows", "1");
	delete Settings;
	return StrToBool(AnimateWindowsEnabled);
}
//---------------------------------------------------------------------------
bool ChkThemeGlowing()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString GlowingEnabled = Settings->ReadString("Theme", "ThemeGlowing", "1");
	delete Settings;
	return StrToBool(GlowingEnabled);
}
//---------------------------------------------------------------------------

//Pobieranie ustawien koloru AlphaControls
int GetHUE()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETHUE, 0, 0);
}
//---------------------------------------------------------------------------
int GetSaturation()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETSATURATION, 0, 0);
}
//---------------------------------------------------------------------------
int GetBrightness()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETBRIGHTNESS, 0, 0);
}
//---------------------------------------------------------------------------

//Sprawdzanie listy ID filmow YouTube do przetworzenia
bool ChkYouTubeListItem()
{
	if(GetYouTubeTitleList->Count) return true;
	else return false;
}
//---------------------------------------------------------------------------

//Pobieranie ID filmu YouTube do przetworzenia
UnicodeString GetYouTubeTitleListItem()
{
	if(GetYouTubeTitleList->Count)
	{
		UnicodeString Item = GetYouTubeTitleList->Strings[0];
		GetYouTubeTitleList->Delete(0);
		return Item;
	}
	else return "";
}
//---------------------------------------------------------------------------

//Dodawanie ID filmu YouTube do listy wykluczonych na czas sesji
void AddToYouTubeExcludeList(UnicodeString ID)
{
	YouTubeExcludeList->Add(ID);
}
//---------------------------------------------------------------------------

//Odswiezanie listy kontaktow
void RefreshList()
{
	PluginLink.CallService(AQQ_SYSTEM_RUNACTION, 0, (LPARAM)L"aRefresh");
}
//---------------------------------------------------------------------------

//Konwersja tekstu na liczbe
int Convert(UnicodeString Char)
{
	for(int IntChar=-113; IntChar<=255; IntChar++)
	{
		if(Char==CHAR(IntChar))
			return IntChar;
	}
	return 0;
}
//---------------------------------------------------------------------------
UnicodeString ConvertToInt(UnicodeString Text)
{
	UnicodeString ConvertedText;
	for(int Count=1; Count<=Text.Length(); Count++)
	{
		UnicodeString tmpStr = Text.SubString(Count, 1);
		int tmpInt = Convert(tmpStr);
		ConvertedText = ConvertedText + IntToStr(tmpInt);
	}
	return ConvertedText;
}
//---------------------------------------------------------------------------

//Kodowanie ciagu znakow do Base64
UnicodeString EncodeBase64(UnicodeString Str)
{
	return (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_BASE64, (WPARAM)Str.w_str(), 3);
}
//---------------------------------------------------------------------------

//Dekodowanie ciagu znakow z Base64
UnicodeString DecodeBase64(UnicodeString Str)
{
	return (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_BASE64, (WPARAM)Str.w_str(), 2);
}
//---------------------------------------------------------------------------

//Skracanie wyswietlania odnosnikow
UnicodeString TrimLinks(UnicodeString Body, bool Status)
{
	//Dodawanie specjalnego tagu do wszystkich linkow
	Body = StringReplace(Body, "<A HREF", "[CC_LINK_START]<A HREF", TReplaceFlags() << rfReplaceAll);
	Body = StringReplace(Body, "</A>", "</A>[CC_LINK_END]", TReplaceFlags() << rfReplaceAll);
	//Formatowanie tresci wiadomosci
	while(Body.Pos("[CC_LINK_END]"))
	{
		//Wyciagniecie kodu HTML odnosnika
		UnicodeString URL = Body;
		URL.Delete(1, URL.Pos("[CC_LINK_START]")+14);
		URL.Delete(URL.Pos("[CC_LINK_END]"), URL.Length());
		//Wyciaganie tekstu odnosnika
		UnicodeString Text = URL;
		Text.Delete(Text.Pos("</A>"), Text.Length());
		Text.Delete(1, Text.Pos("\">")+1);
		//Link do filmu YouTube (tylko dla opisow)
		if((Status)&&(((Text.Pos("youtube.com"))&&(((Text.Pos("watch?"))&&(Text.Pos("v=")))||(Text.Pos("/v/"))))||(Text.Pos("youtu.be"))))
		{
			//Zmienna ID
			UnicodeString ID;
			//Wyciaganie ID - fullscreenowy
			if(Text.Pos("/v/"))
			{
				//Parsowanie ID
				ID = Text;
				ID.Delete(1, ID.Pos("/v/")+2);
			}
			//Wyciaganie ID - zwykly & mobilny
			else if(Text.Pos("youtube.com"))
			{
				//Parsowanie ID
				ID = Text;
				ID.Delete(1, ID.Pos("v=")+1);
				if(ID.Pos("&"))	ID.Delete(ID.Pos("&"), ID.Length());
				if(ID.Pos("#"))	ID.Delete(ID.Pos("#"), ID.Length());
			}
			//Wyciaganie ID - skrocony
			else if(Text.Pos("youtu.be"))
			{
				//Parsowanie ID
				ID = Text;
				ID.Delete(1, ID.Pos(".be/")+3);
			}
			//Id nie znajduje sie na liscie ID filmow YouTube wykluczonych na czas sesji
			if(YouTubeExcludeList->IndexOf(ID)==-1)
			{
				//Szukanie ID w cache
				TIniFile *Ini = new TIniFile(SessionFileDir);
				UnicodeString Title = DecodeBase64(Ini->ReadString("YouTube", ConvertToInt(ID), ""));
				delete Ini;
				//Tytul pobrany z cache
				if(!Title.IsEmpty())
				{
					//Odnosnik z parametrem title
					if(URL.Pos("title="))
						Body = StringReplace(Body, "\">" + Text, "\">" + Title, TReplaceFlags());
					//Odnosnik bez parametru title
					else
						Body = StringReplace(Body, "\">" + Text, "\" title=\"" + Text + "\">" + Title, TReplaceFlags());
				}
				//Brak tytulu w cache
				else
				{
					//Przypisanie uchwytu do formy ustawien
					if(!hSettingsForm)
					{
						Application->Handle = (HWND)SettingsForm;
						hSettingsForm = new TSettingsForm(Application);
					}
					//Dodanie ID do przetworzenia
					GetYouTubeTitleList->Add(ID);
					//Wlaczenie watku
					if(!hSettingsForm->GetYouTubeTitleThread->Active) hSettingsForm->GetYouTubeTitleThread->Start();
					//Odnosnik z parametrem title
					if(URL.Pos("title="))
						Body = StringReplace(Body, "\">" + Text, "\">["+GetLangStr("YouTubeTemp")+"...]", TReplaceFlags());
					//Odnosnik bez parametru title
					else
						Body = StringReplace(Body, "\">" + Text, "\" title=\"" + Text + "\">["+GetLangStr("YouTubeTemp")+"...]", TReplaceFlags());
				}
			}
			//Przejscie do normalnego skracana linkow
			else goto NormalTrim;
		}
		//Inne linki
		else
		{
			//Skok do normnalnego skracania linkow
			NormalTrim: { /* Only Jump */ }
			//Wycinanie domeny z adresu URL
			UnicodeString Domain = Text;
			if(Domain.LowerCase().Pos("http://"))
			{
				Domain.Delete(1, Domain.LowerCase().Pos("http://")+6);
				if(Domain.Pos("/")) Domain.Delete(Domain.Pos("/"), Domain.Length());
			}
			else if(Domain.LowerCase().Pos("https://"))
			{
				Domain.Delete(1, Domain.LowerCase().Pos("https://")+7);
				if(Domain.Pos("/")) Domain.Delete(Domain.Pos("/"), Domain.Length());
			}
			else if(Domain.LowerCase().Pos("www."))
			{
				Domain.Delete(1, Domain.LowerCase().Pos("www.")+3);
				if(Domain.Pos("/")) Domain.Delete(Domain.Pos("/"), Domain.Length());
			}
			else Domain = "";
			//Wyciagnieto prawidlowo nazwe domeny z adresu URL
			if(!Domain.IsEmpty())
			{
				//Usuniecie subdomeny WWW
				if(Domain.LowerCase().Pos("www.")) Domain.Delete(Domain.LowerCase().Pos("www."), Domain.LowerCase().Pos("www.")+3);
				//Odnosnik z parametrem title
				if(URL.Pos("title="))
					Body = StringReplace(Body, "\">" + Text, "\">[" + Domain + "]", TReplaceFlags());
				//Odnosnik bez parametru title
				else
					Body = StringReplace(Body, "\">" + Text, "\" title=\"" + Text + "\">[" + Domain + "]", TReplaceFlags());
			}
		}
		//Usuwanie wczesniej dodanych tagow
		Body = StringReplace(Body, "[CC_LINK_START]", "", TReplaceFlags());
		Body = StringReplace(Body, "[CC_LINK_END]", "", TReplaceFlags());
	}
	return Body;
}
//---------------------------------------------------------------------------

//Hook na pokazywane wiadomosci
INT_PTR __stdcall OnAddLine(WPARAM wParam, LPARAM lParam)
{
	//Skracanie wiadomosci wlaczone / komunikator nie jest zamykany
	if((TrimMessages)&&(!ForceUnloadExecuted))
	{
		//Pobieranie danych wiadomosci
		TPluginMessage AddLineMessage = *(PPluginMessage)lParam;
		//Pobieranie sformatowanej tresci wiadomosci
		UnicodeString Body = (wchar_t*)AddLineMessage.Body;
		//Zapisywanie oryginalnej tresci wiadomosci
		UnicodeString BodyOrg = Body;
		//Skracanie wyswietlania odnosnikow
		Body = TrimLinks(Body, false);
		//Zmienianie tresci wiadomosci
		if(Body!=BodyOrg)
		{
			AddLineMessage.Body = Body.w_str();
			memcpy((PPluginMessage)lParam, &AddLineMessage, sizeof(TPluginMessage));
			return 2;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na wylaczenie komunikatora poprzez usera
INT_PTR __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam)
{
	//Info o rozpoczeciu procedury zamykania komunikatora
	ForceUnloadExecuted = true;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane kolorystyki AlphaControls
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam)
{
	//Komunikator nie jest zamykany
	if(!ForceUnloadExecuted)
	{
		//Okno ustawien zostalo juz stworzone
		if(hSettingsForm)
		{
			//Wlaczona zaawansowana stylizacja okien
			if(ChkSkinEnabled())
			{
				TPluginColorChange ColorChange = *(PPluginColorChange)wParam;
				hSettingsForm->sSkinManager->HueOffset = ColorChange.Hue;
				hSettingsForm->sSkinManager->Saturation = ColorChange.Saturation;
				hSettingsForm->sSkinManager->Brightness = ColorChange.Brightness;
			}
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane lokalizacji
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam)
{
	//Czyszczenie cache lokalizacji
	ClearLngCache();
	//Pobranie sciezki do katalogu prywatnego uzytkownika
	UnicodeString PluginUserDir = GetPluginUserDir();
	//Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)lParam;
	LangPath = PluginUserDir + "\\\\Languages\\\\TrimLinks\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE, 0, 0);
		LangPath = PluginUserDir + "\\\\Languages\\\\TrimLinks\\\\" + LangCode + "\\\\";
	}
	//Aktualizacja lokalizacji form wtyczki
	for(int i=0; i<Screen->FormCount; i++)
		LangForm(Screen->Forms[i]);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane widocznego opisu kontatku na liscie kontatkow
INT_PTR __stdcall OnSetHTMLStatus(WPARAM wParam, LPARAM lParam)
{
	//Skracanie opisow wlaczone / komunikator nie jest zamykany
	if((TrimStatus)&&(!ForceUnloadExecuted))
	{
		//Pobieranie sformatowanego opisu
		UnicodeString Body = (wchar_t*)lParam;
		//Jezeli opis cos zawiera
		if(!Body.IsEmpty())
		{
			//Zapisywanie oryginalnego opisu
			UnicodeString BodyOrg = Body;
			//Skracanie wyswietlania odnosnikow
			Body = TrimLinks(Body, true);
			//Zmienianie opisu na liscie kontatkow
			if(Body!=BodyOrg)
				return (LPARAM)Body.w_str();
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane kompozycji
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam)
{
	//Pobieranie sciezki nowej aktywnej kompozycji
	UnicodeString ThemeDir = StringReplace((wchar_t*)lParam, "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
	//Okno ustawien zostalo juz stworzone
	if(hSettingsForm)
	{
		//Wlaczona zaawansowana stylizacja okien
		if(ChkSkinEnabled())
		{
			UnicodeString ThemeSkinDir = ThemeDir + "\\\\Skin";
			//Plik zaawansowanej stylizacji okien istnieje
			if(FileExists(ThemeSkinDir + "\\\\Skin.asz"))
			{
				//Dane pliku zaawansowanej stylizacji okien
				ThemeSkinDir = StringReplace(ThemeSkinDir, "\\\\", "\\", TReplaceFlags() << rfReplaceAll);
				hSettingsForm->sSkinManager->SkinDirectory = ThemeSkinDir;
				hSettingsForm->sSkinManager->SkinName = "Skin.asz";
				//Ustawianie animacji AlphaControls
				if(ChkThemeAnimateWindows()) hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 200;
				else hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 0;
				hSettingsForm->sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
				//Zmiana kolorystyki AlphaControls
				hSettingsForm->sSkinManager->HueOffset = GetHUE();
				hSettingsForm->sSkinManager->Saturation = GetSaturation();
				hSettingsForm->sSkinManager->Brightness = GetBrightness();
				//Aktywacja skorkowania AlphaControls
				hSettingsForm->sSkinManager->Active = true;
			}
			//Brak pliku zaawansowanej stylizacji okien
			else hSettingsForm->sSkinManager->Active = false;
		}
		//Zaawansowana stylizacja okien wylaczona
		else hSettingsForm->sSkinManager->Active = false;
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie/otwarcie okien
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	//Komunikator nie jest zamykany
	if(!ForceUnloadExecuted)
	{
		//Pobranie informacji o oknie i eventcie
		TPluginWindowEvent WindowEvent = *(PPluginWindowEvent)lParam;
		int Event = WindowEvent.WindowEvent;
		UnicodeString ClassName = (wchar_t*)WindowEvent.ClassName;
		//Otworzenie okna instalowania dodatku
		if((ClassName=="TfrmInstallAddon")&&(Event==WINDOW_EVENT_CREATE))
		{
			//Informacja o otwarciu okna instalowania dodatku
			FrmInstallAddonExist = true;
		}
		//Zamkniecie okna instalowania dodatku
		if((ClassName=="TfrmInstallAddon")&&(Event==WINDOW_EVENT_CLOSE))
		{
			//Informacja o zamknieciu okna instalowania dodatku
			FrmInstallAddonExist = false;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Zapisywanie zasobów
void ExtractRes(wchar_t* FileName, wchar_t* ResName, wchar_t* ResType)
{
	TPluginTwoFlagParams PluginTwoFlagParams;
	PluginTwoFlagParams.cbSize = sizeof(TPluginTwoFlagParams);
	PluginTwoFlagParams.Param1 = ResName;
	PluginTwoFlagParams.Param2 = ResType;
	PluginTwoFlagParams.Flag1 = (int)HInstance;
	PluginLink.CallService(AQQ_FUNCTION_SAVERESOURCE, (WPARAM)&PluginTwoFlagParams, (LPARAM)FileName);
}
//---------------------------------------------------------------------------

//Obliczanie sumy kontrolnej pliku
UnicodeString MD5File(UnicodeString FileName)
{
	if(FileExists(FileName))
	{
		UnicodeString Result;
		TFileStream *fs;
		fs = new TFileStream(FileName, fmOpenRead | fmShareDenyWrite);
		try
		{
			TIdHashMessageDigest5 *idmd5= new TIdHashMessageDigest5();
			try
			{
				Result = idmd5->HashStreamAsHex(fs);
			}
			__finally
			{
				delete idmd5;
			}
		}
		__finally
		{
			delete fs;
		}
		return Result;
	}
	else return 0;
}
//---------------------------------------------------------------------------

//Odczyt ustawien
void LoadSettings()
{
	TIniFile *Ini = new TIniFile(GetPluginUserDir()+"\\\\TrimLinks\\\\Settings.ini");
	TrimMessages = Ini->ReadBool("Trim", "Messages", true);
	bool pTrimStatus = TrimStatus;
	TrimStatus = Ini->ReadBool("Trim", "Status", true);
	if((pTrimStatus!=TrimStatus)&&(!LoadExecuted)) RefreshList();
	delete Ini;
}
//---------------------------------------------------------------------------

extern "C" INT_PTR __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
	//Info o rozpoczeciu procedury ladowania
	LoadExecuted = true;
	//Linkowanie wtyczki z komunikatorem
	PluginLink = *Link;
	//Pobranie sciezki do katalogu prywatnego uzytkownika
	UnicodeString PluginUserDir = GetPluginUserDir();
	//Tworzenie katalogow lokalizacji
	if(!DirectoryExists(PluginUserDir+"\\\\Languages"))
		CreateDir(PluginUserDir+"\\\\Languages");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\TrimLinks"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\TrimLinks");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL");
	//Wypakowanie plikow lokalizacji
	//547FCF355947EB7F9482BA268D77B0AB
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\Const.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\Const.lng").w_str(), L"EN_CONST", L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\Const.lng")!="547FCF355947EB7F9482BA268D77B0AB")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\Const.lng").w_str(), L"EN_CONST", L"DATA");
	//21B9321BBABC5EAFAFF79EDE6B7E5B2B
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\TSettingsForm.lng").w_str(), L"EN_SETTINGSFRM", L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\TSettingsForm.lng")!="21B9321BBABC5EAFAFF79EDE6B7E5B2B")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\EN\\\\TSettingsForm.lng").w_str(), L"EN_SETTINGSFRM", L"DATA");
	//FCDFE35C524D53FE8F73D2FAA4DD3B80
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\Const.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\Const.lng").w_str(), L"PL_CONST", L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\Const.lng")!="FCDFE35C524D53FE8F73D2FAA4DD3B80")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\Const.lng").w_str(), L"PL_CONST", L"DATA");
	//CF18B7B4F3663892A5FDEB4E3E364115
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\TSettingsForm.lng").w_str(), L"PL_SETTINGSFRM", L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\TSettingsForm.lng")!="CF18B7B4F3663892A5FDEB4E3E364115")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\TrimLinks\\\\PL\\\\TSettingsForm.lng").w_str(), L"PL_SETTINGSFRM", L"DATA");
	//Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETLANGCODE, 0, 0);
	LangPath = PluginUserDir + "\\\\Languages\\\\TrimLinks\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE, 0, 0);
		LangPath = PluginUserDir + "\\\\Languages\\\\TrimLinks\\\\" + LangCode + "\\\\";
	}
	//Wypakiwanie ikonki TrimLinks.dll.png
	//AE97BBAABA1C385A8FD93D66723653DE
	if(!DirectoryExists(PluginUserDir+"\\\\Shared"))
		CreateDir(PluginUserDir+"\\\\Shared");
	if(!FileExists(PluginUserDir+"\\\\Shared\\\\TrimLinks.dll.png"))
		ExtractRes((PluginUserDir+"\\\\Shared\\\\TrimLinks.dll.png").w_str(), L"SHARED", L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Shared\\\\TrimLinks.dll.png")!="AE97BBAABA1C385A8FD93D66723653DE")
		ExtractRes((PluginUserDir+"\\\\Shared\\\\TrimLinks.dll.png").w_str(), L"SHARED", L"DATA");
	//Pobranie sciezki do pliku sesji
	SessionFileDir = PluginUserDir + "\\\\TrimLinks\\\\Session.ini";
	//Folder z ustawieniami wtyczki
	if(!DirectoryExists(PluginUserDir + "\\\\TrimLinks"))
		CreateDir(PluginUserDir + "\\\\TrimLinks");
	//Wczytanie ustawien
	LoadSettings();
	//Hook na pokazywane wiadomosci
	PluginLink.HookEvent(AQQ_CONTACTS_ADDLINE, OnAddLine);
	//Hook na wylaczenie komunikatora poprzez usera
	PluginLink.HookEvent(AQQ_SYSTEM_BEFOREUNLOAD, OnBeforeUnload);
	//Hook na zmiane kolorystyki AlphaControls
	PluginLink.HookEvent(AQQ_SYSTEM_COLORCHANGEV2, OnColorChange);
	//Hook na zmiane lokalizacji
	PluginLink.HookEvent(AQQ_SYSTEM_LANGCODE_CHANGED, OnLangCodeChanged);
	//Hook na zmiane widocznego opisu kontatku na liscie kontatkow
	PluginLink.HookEvent(AQQ_CONTACTS_SETHTMLSTATUS, OnSetHTMLStatus);
	//Hook na zmiane kompozycji
	PluginLink.HookEvent(AQQ_SYSTEM_THEMECHANGED, OnThemeChanged);
	//Hook na zamkniecie/otwarcie okien
	PluginLink.HookEvent(AQQ_SYSTEM_WINDOWEVENT, OnWindowEvent);
	//Wszystkie moduly zostaly zaladowane
	if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED, 0, 0))
		//Odswiezenie listy kontaktow
		RefreshList();
	//Info o zakonczeniu procedury ladowania
	LoadExecuted = false;

	return 0;
}
//---------------------------------------------------------------------------

//Wyladowanie wtyczki
extern "C" INT_PTR __declspec(dllexport) __stdcall Unload()
{
	//Wyladowanie wszystkich hookow
	PluginLink.UnhookEvent(OnAddLine);
	PluginLink.UnhookEvent(OnBeforeUnload);
	PluginLink.UnhookEvent(OnColorChange);
	PluginLink.UnhookEvent(OnLangCodeChanged);
	PluginLink.UnhookEvent(OnSetHTMLStatus);
	PluginLink.UnhookEvent(OnThemeChanged);
	PluginLink.UnhookEvent(OnWindowEvent);
	//Odswiezenie listy kontaktow
	if((!ForceUnloadExecuted)&&(!FrmInstallAddonExist)) RefreshList();

	return 0;
}
//---------------------------------------------------------------------------

//Ustawienia wtyczki
extern "C" INT_PTR __declspec(dllexport)__stdcall Settings()
{
	//Przypisanie uchwytu do formy ustawien
	if(!hSettingsForm)
	{
		Application->Handle = (HWND)SettingsForm;
		hSettingsForm = new TSettingsForm(Application);
	}
	//Pokaznie okna ustawien
	hSettingsForm->Show();

	return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" PPluginInfo __declspec(dllexport) __stdcall AQQPluginInfo(DWORD AQQVersion)
{
	PluginInfo.cbSize = sizeof(TPluginInfo);
	PluginInfo.ShortName = L"TrimLinks";
	PluginInfo.Version = PLUGIN_MAKE_VERSION(1,4,1,2);
	PluginInfo.Description = L"Skraca wszystkie odnoœniki w oknie rozmowy jak i te na liœcie kontaktów do znaczniej wygodniejszej formy.";
	PluginInfo.Author = L"Krzysztof Grochocki";
	PluginInfo.AuthorMail = L"kontakt@beherit.pl";
	PluginInfo.Copyright = L"Krzysztof Grochocki";
	PluginInfo.Homepage = L"http://beherit.pl";
	PluginInfo.Flag = 0;
	PluginInfo.ReplaceDefaultModule = 0;

	return &PluginInfo;
}
//---------------------------------------------------------------------------
