/******************************************************************************
 * WINDOWS/W_MOUSE.C
 ******************************************************************************/

#include <w_mouse.h>
#include <windows_.h>

LOCAL _WORD mouse_calls;

/******************************************************************************/
/* HideMouse()																  */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Schaltet die Maus aus, wenn sie sichtbar ist. Sollte immer aufge-		  */
/* rufen werden, bevor gezeichnet wird.                                       */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/*					 keine													  */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/*					 keine													  */
/******************************************************************************/

GLOBAL _VOID HideMouse(_VOID)
{
	/* ShowCursor(FALSE); */
	mouse_calls++;
}

/******************************************************************************/
/* ShowMouse()																  */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Zeigt die Maus an, wenn genauso viel Aufrufe wie bei HideMouse()           */
/* erfolgt sind.															  */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/*					 keine													  */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/*					 keine													  */
/******************************************************************************/

GLOBAL _VOID ShowMouse(_VOID)
{
	/* ShowCursor(TRUE); */
	mouse_calls--;
}

/******************************************************************************/
/* GetMouseCalls()															  */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Zeigt die Maus sofort an, egal wieviel Aufrufe von HideMouse()			  */
/* erfolgt sind.															  */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/*					 keine													  */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/* Anzahl der Aufrufe die fuer HideMouse noetig sind, um den alten			  */
/* Zustand wieder herzustellen												  */
/******************************************************************************/

GLOBAL _WORD GetMouseCalls(_VOID)
{
	_WORD count = mouse_calls;

	mouse_calls = 0;
	return count;
}

/******************************************************************************/
/* SetMouseCalls()															  */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Restauriert den Zustand vor GetMouseCalls()								  */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/* -> count        = Wert, der von GetMouseCalls() zurueckgegeben			  */
/*					 wurde													  */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/*					 keine													  */
/******************************************************************************/

GLOBAL _VOID SetMouseCalls(_WORD count)
{
	mouse_calls = count;
}

/******************************************************************************/
/* SetMouse()																  */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Setzt den Mauszeiger fuer alle Fenster auf eine bestimmte Form			  */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/* -> form		   = Nummer fuer das Mauszeiger                               */
/*	 moegliche Werte:														  */
/*	   MOUSE_RESET:     Setzt alle Zaehler zurueck und stellt die			  */
/*						normale Form dar									  */
/*	   MOUSE_RESTORE   :Stellt den letzten Zustand wieder her				  */
/*	   MOUSE_NORMAL    :Der Standard-Zeiger (Pfeil)                           */
/*	   MOUSE_MENU	   :Mauszeiger, wenn sich die Maus in einem               */
/*						Menue befindet										  */
/*	   MOUSE_DIALOG    :Mauszeiger, wenn sich die Maus in einem               */
/*						Dialog befindet                                       */
/*	   MOUSE_BUSY	   :Form, die anzeigt dass das System					  */
/*						beschaeftigt ist									  */
/*	   MOUSE_MOVING    :Schiebende Hand o.ae.								  */
/*	   MOUSE_SIZE_NESW :Pfeil von Nord-Ost nach Sued-West					  */
/*	   MOUSE_SIZE_SWNE :Pfeil von Sued-West nach Nord-Ost					  */
/*	   MOUSE_SIZE_NWSE :Pfeil von Nord-West nach Sued-Ost					  */
/*	   MOUSE_SIZE_SENW :Pfeil von Sued-Ost nach Nord-West					  */
/*	   MOUSE_SIZE_NS   :Pfeil von Norden nach Sueden						  */
/*	   MOUSE_SIZE_SN   :Pfeil von Sueden nach Norden						  */
/*	   MOUSE_SIZE_WE   :Pfeil von Westen nach Osten                           */
/*	   MOUSE_SIZE_EW   :Pfeil von Osten nach Westen                           */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/*					 keine													  */
/******************************************************************************/

#define MAX_MOUSE_DEPTH 100
LOCAL struct {
	HCURSOR hcursor;
	_BOOL destroy;
} savetab[MAX_MOUSE_DEPTH];
LOCAL _WORD saveidx;

GLOBAL _VOID SetMouse(_WORD form)
{
	LOCAL _WORD mform;
	HCURSOR hform = NULL;
	_BOOL destroy = FALSE;

	switch (form)
	{
	case MOUSE_UNKNOWN:
		saveidx = 0;
		mform = MOUSE_UNKNOWN;
		form = MOUSE_NORMAL;
		break;
	case MOUSE_RESET:
		while (saveidx > 0)
		{
			HCURSOR old;

			--saveidx;
			old = SetCursor(savetab[saveidx].hcursor);
			if (savetab[saveidx].destroy)
				DestroyCursor(old);
		}
		saveidx = 0;
		mform = MOUSE_UNKNOWN;
		form = MOUSE_NORMAL;
		break;
	case MOUSE_RESTORE:
		if (saveidx > 0)
		{
			HCURSOR old;

			--saveidx;
			old = SetCursor(savetab[saveidx].hcursor);
			if (savetab[saveidx].destroy)
				DestroyCursor(old);
		}
		break;
	case MOUSE_MENU:
	case MOUSE_NORMAL:
	case MOUSE_DIALOG:
		hform = LoadCursor(NULL, IDC_ARROW);
		break;
	case MOUSE_BUSY:
		hform = LoadCursor(NULL, IDC_WAIT);
		break;
	case MOUSE_MOVING:
		/* hform = LoadCursor(GetInstance(), MAKEINTRESOURCE(IDC_PANEBOTH)); */
		destroy = TRUE;
		break;
	case MOUSE_POINTING:
		hform = LoadCursor(NULL, IDC_UPARROW);
		break;
	case MOUSE_SIZE_NESW:
	case MOUSE_SIZE_SWNE:
		hform = LoadCursor(NULL, IDC_SIZENESW);
		break;
	case MOUSE_SIZE_NWSE:
	case MOUSE_SIZE_SENW:
		hform = LoadCursor(NULL, IDC_SIZENWSE);
		break;
	case MOUSE_SIZE_NS:
	case MOUSE_SIZE_SN:
	case MOUSE_MOVE_NS:
	case MOUSE_MOVE_SN:
		hform = LoadCursor(NULL, IDC_SIZENS);
		break;
	case MOUSE_SIZE_EW:
	case MOUSE_SIZE_WE:
	case MOUSE_MOVE_WE:
	case MOUSE_MOVE_EW:
		hform = LoadCursor(NULL, IDC_SIZEWE);
		break;
	default:
		hform = LoadCursor(NULL, IDC_ARROW);
		break;
	}
	if (hform != NULL)
	{
		if (saveidx < MAX_MOUSE_DEPTH)
		{
			savetab[saveidx].hcursor = SetCursor(hform);
			savetab[saveidx].destroy = destroy;
			saveidx++;
		} else
		{
			if (destroy)
				DestroyCursor(hform);
		}
	}
	mform = form;
}

/******************************************************************************/
/* SetMouse_Shape()                                                           */
/* -------------------------------------------------------------------------- */
/* Entwickler......: Th.Otto												  */
/* 1.Version.......:														  */
/* letzte Aenderung:														  */
/* -------------------------------------------------------------------------- */
/* Setzt den Mauszeiger fuer alle Fenster auf ein beliebiges Bild.			  */
/* Wird momentan nur von ORCS zum testen eines Zeigers verwendet			  */
/* -------------------------------------------------------------------------- */
/* Parameter:																  */
/* -> data		   = Daten der Bitmap										  */
/* -> w            = Breite der Bitmap in Pixel                               */
/* -> h            = Hoehe der Bitmap in Pixel								  */
/* -> color        = Farbnummer                                               */
/* -------------------------------------------------------------------------- */
/* Rueckgabe:																  */
/*					 TRUE wenn der Zeiger gesetzt werden konnte               */
/*					 FALSE bei Fehler										  */
/******************************************************************************/

GLOBAL _BOOL SetMouse_Shape(_UBYTE *data, _UBYTE *mask, _WORD w, _WORD h, _WORD color)
{
	HCURSOR hcurs;

	UNUSED(color);
	if (data == NULL)
	{
		--saveidx;
		hcurs = SetCursor(savetab[saveidx].hcursor);
		if (savetab[saveidx].destroy)
			DestroyCursor(hcurs);
	} else
	{
		if (saveidx >= MAX_MOUSE_DEPTH)
			return FALSE;
		if (w != GetSystemMetrics(SM_CXCURSOR) || h != GetSystemMetrics(SM_CYCURSOR))
			return FALSE;
		if (mask == NULL)
			mask = data;
		hcurs = CreateCursor(GetInstance(), 0, 0, w, h, mask, data);
		savetab[saveidx].hcursor = SetCursor(hcurs);;
		savetab[saveidx].destroy = TRUE;
		saveidx++;
	}
	return TRUE;
}
