
see [incredibuild xml interface](https://www.incredibuild.com/webhelp/xml_notes.html),
[automatic interception interface profile](https://www.incredibuild.com/webhelp/profile_xml_structure.html).

# Profile Syntax

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<Profile FormatVersion="1">
    <Tools>
        <Tool Filename="MainProcess" AllowIntercept="true"  DeriveCaptionFrom="lastparam" AutoReserveMemory="*.gch" />
        <Tool Filename="DummySubProcess" AllowRemote="true" />
        <Tool Filename="link" AllowRemote="false" />
    </Tools>
</Profile>
```

    IBConsole  /command="MainProcess.exe DummySubProcess.exe 10"  /profile="profile.xml"

