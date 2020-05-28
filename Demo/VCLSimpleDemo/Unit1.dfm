object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'Form1'
  ClientHeight = 214
  ClientWidth = 265
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 32
    Top = 16
    Width = 52
    Height = 13
    Caption = 'Font Name'
  end
  object lblClock: TLabel
    AlignWithMargins = True
    Left = 3
    Top = 64
    Width = 259
    Height = 147
    Margins.Top = 64
    Align = alClient
    Alignment = taCenter
    Caption = '--:--:--'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -53
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Layout = tlCenter
    ExplicitWidth = 152
    ExplicitHeight = 64
  end
  object comboboxFontName: TComboBox
    Left = 32
    Top = 32
    Width = 145
    Height = 21
    Style = csDropDownList
    TabOrder = 0
    OnChange = comboboxFontNameChange
  end
  object Timer1: TTimer
    Interval = 300
    OnTimer = Timer1Timer
    Left = 208
    Top = 16
  end
end
