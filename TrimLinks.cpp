//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include "Aqq.h"
//---------------------------------------------------------------------------

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
PPluginContact Contact;
PPluginMessage Message;
//FORWARD-AQQ-HOOKS----------------------------------------------------------
//int __stdcall OnAddLine(WPARAM wParam, LPARAM lParam);
//int __stdcall OnSetHTMLStatus(WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Skracanie wyswietlania odnosnikow
UnicodeString TrimBodyLinks(UnicodeString Body)
{
  //Dodawanie specjalnego tagu do wszystkich linkow
  Body = StringReplace(Body, "</A>", "[CC_LINK_END]</A>", TReplaceFlags() << rfReplaceAll);
  //Formatowanie tresci wiadomosci
  while(Body.Pos("[CC_LINK_END]"))
  {
	//Link with [CC_LINK_END] tag
	UnicodeString URL_WithTag = Body;
	URL_WithTag.Delete(URL_WithTag.Pos("[CC_LINK_END]")+13,URL_WithTag.Length());
	while(URL_WithTag.Pos("\">")) URL_WithTag.Delete(1,URL_WithTag.Pos("\">")+1);
	//Link without [CC_LINK_END] tag
	UnicodeString URL_WithOutTag = URL_WithTag;
	URL_WithOutTag.Delete(URL_WithOutTag.Pos("[CC_LINK_END]"),URL_WithOutTag.Length());
	//Wycinanie domeny z adresow URL
	UnicodeString URL_OnlyDomain = URL_WithOutTag;
	if(URL_OnlyDomain.LowerCase().Pos("www."))
	{
	  URL_OnlyDomain.Delete(1,URL_OnlyDomain.LowerCase().Pos("www.")+3);
	  if(URL_OnlyDomain.Pos("/"))
	   URL_OnlyDomain.Delete(URL_OnlyDomain.Pos("/"),URL_OnlyDomain.Length());
	  //Formatowanie linku
	  Body = StringReplace(Body, URL_WithOutTag + "\">" + URL_WithTag, URL_WithOutTag + "\" title=\"" + URL_WithOutTag.Trim() + "\">["+ URL_OnlyDomain + "]", TReplaceFlags());
	}
	else if(URL_OnlyDomain.LowerCase().Pos("http://"))
	{
	  URL_OnlyDomain.Delete(1,URL_OnlyDomain.LowerCase().Pos("http://")+6);
	  if(URL_OnlyDomain.Pos("/"))
	   URL_OnlyDomain.Delete(URL_OnlyDomain.Pos("/"),URL_OnlyDomain.Length());
	  //Formatowanie linku
	  Body = StringReplace(Body, URL_WithOutTag + "\">" + URL_WithTag, URL_WithOutTag + "\" title=\"" + URL_WithOutTag.Trim() + "\">["+ URL_OnlyDomain + "]", TReplaceFlags());
	}
	else if(URL_OnlyDomain.LowerCase().Pos("https://"))
	{
	  URL_OnlyDomain.Delete(1,URL_OnlyDomain.LowerCase().Pos("https://")+7);
	  if(URL_OnlyDomain.Pos("/"))
	   URL_OnlyDomain.Delete(URL_OnlyDomain.Pos("/"),URL_OnlyDomain.Length());
	  //Formatowanie linku
	  Body = StringReplace(Body, URL_WithOutTag + "\">" + URL_WithTag, URL_WithOutTag + "\" title=\"" + URL_WithOutTag.Trim() + "\">["+ URL_OnlyDomain + "]", TReplaceFlags());
	}
	//Niestandardowy odnosnik
	else
	 Body = StringReplace(Body, "[CC_LINK_END]", "", TReplaceFlags());
  }
  return Body;
}
//---------------------------------------------------------------------------

//Hook na pokazywane wiadomosci
int __stdcall OnAddLine(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontatku
  Contact = (PPluginContact)wParam;
  //Pobieranie identyfikatora kontatku
  UnicodeString ContactJID = (wchar_t*)Contact->JID;
  //Kontakt nie jest botem Blip
  if((ContactJID!="blip@blip.pl")&&(ContactJID.Pos("202@plugin.gg")!=1))
  {
	//Pobieranie danych wiadomosci
	Message = (PPluginMessage)lParam;
	//Pobieranie sformatowanej tresci wiadomosci
	UnicodeString Body = (wchar_t*)Message->Body;
	//Zapisywanie oryginalnej tresci wiadomosci
	UnicodeString BodyOrg = Body;
	//Skracanie wyswietlania odnosnikow
	Body = TrimBodyLinks(Body);
	//Zmienianie tresci wiadomosci
	if(Body!=BodyOrg)
	{
	  Message->Body = Body.w_str();
	  lParam = (LPARAM)Message;
	  return 2;
	}  
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane widocznego opisu kontatku na liscie kontatkow
int __stdcall OnSetHTMLStatus(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie sformatowanego opisu
  UnicodeString Body = (wchar_t*)lParam;
  //Jezeli opis cos zawiera
  if(!Body.IsEmpty())
  {
	//Zapisywanie oryginalnego opisu
	UnicodeString BodyOrg = Body;
	//Skracanie wyswietlania odnosnikow
	Body = TrimBodyLinks(Body);
	//Zmienianie opisu na liscie kontatkow
	if(Body!=BodyOrg)
	 return (LPARAM)Body.w_str();
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Hook na pokazywane wiadomosci
  PluginLink.HookEvent(AQQ_CONTACTS_ADDLINE,OnAddLine);
  //Hook na zmiane widocznego opisu kontatku na liscie kontatkow
  PluginLink.HookEvent(AQQ_CONTACTS_SETHTMLSTATUS,OnSetHTMLStatus);
  //Wszystkie moduly zostaly zaladowane
  if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
   //Odswiezenie listy kontaktow
   PluginLink.CallService(AQQ_SYSTEM_RUNACTION,0,(LPARAM)L"aRefresh");

  return 0;
}
//---------------------------------------------------------------------------

//Wyladowanie wtyczki
extern "C" int __declspec(dllexport) __stdcall Unload()
{
  //Wyladowanie wszystkich hookow
  PluginLink.UnhookEvent(OnAddLine);
  PluginLink.UnhookEvent(OnSetHTMLStatus);
  //Usuniecie wskaznikow do struktur
  delete Contact;
  delete Message;
  //Odswiezenie listy kontaktow
  PluginLink.CallService(AQQ_SYSTEM_RUNACTION,0,(LPARAM)L"aRefresh");

  return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"TrimLinks";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.Description = L"Skracanie wyœwietlania odnoœników do wygodniejszej formy";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";

  return &PluginInfo;
}
//---------------------------------------------------------------------------
