program kyfeep;

uses
  QForms,
  main in 'main.pas' {mainform},
  data in 'data.pas' {DataMod: TDataModule};

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(Tmainform, mainform);
  Application.CreateForm(TDataMod, DataMod);
  Application.Run;
end.
