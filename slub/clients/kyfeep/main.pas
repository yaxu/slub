unit main;

interface

uses
  SysUtils, Types, Classes, Variants, QGraphics, QControls, QForms, QDialogs,
  QStdCtrls, QExtCtrls, QButtons, QFeep, QGrids;

type
  Tmainform = class(TForm)
    mainpanel: TPanel;
    feeppanel: TPanel;
    SpeedButton2: TSpeedButton;
    feepname: TLabel;
    nameedit: TEdit;
    typecombo: TComboBox;
    Label1: TLabel;
    StringGrid1: TStringGrid;
    PaintBox1: TPaintBox;
    procedure SpeedButton2Click(Sender: TObject);
    procedure FeepDblClick(Sender: TObject);
    procedure PaintBox1Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  mainform: Tmainform;

implementation

{$R *.xfm}

procedure Tmainform.SpeedButton2Click(Sender: TObject);
begin
     mainform.feeppanel.visible := false;
end;

procedure Tmainform.FeepDblClick(Sender: TObject);
  var
    mylabel : TLabel;
    spos : Integer;
begin
     mylabel := Sender as TLabel;
     feeppanel.Top := mylabel.Top;
     feeppanel.left := mylabel.Left;
     feeppanel.Visible := true;
     nameedit.Text := mylabel.Caption;
     nameedit.Enabled := false;
     typecombo.enabled := false;
     spos := pos('-', mylabel.Caption);
     nameedit.Text := copy(mylabel.Caption, 1, spos - 2);
     typecombo.Text := copy(mylabel.Caption, spos + 2, 127);
end;

procedure Tmainform.PaintBox1Click(Sender: TObject);
begin
   with canvas do begin
        application.messagebox('foo');
        pen.Color := random(65535);
        moveto(0,0);
        lineto(300,300);
   end;

end;

end.
