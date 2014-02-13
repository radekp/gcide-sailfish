#include "dict.h"

Dict::Dict(QObject* parent)
    : QObject(parent)
    , dictFile("/usr/share/gcide/gcide-entries.xml")
{
}

// Compare current key and searched expression.
static int compareExprKey(const QString &expr, const QString &key)
{
    for(int i = 0; i < expr.length(); i++)
    {
        if(i >= key.length())
        {
            return 1;       // expression is bigger
        }
        QChar ech = expr.at(i).toUpper();
        QChar kch = key.at(i).toUpper();
        if(ech == kch)
        {
            continue;
        }
        if(!kch.isLetterOrNumber() || kch.unicode() > 128)
        {
            return UNCOMPARABLE_CHARS;
        }
        return ech.unicode() - kch.unicode();
    }
    return 0;
}

QString Dict::searchExpr(const QString &expr, int maxResults)
{
    if(!dictFile.open(QFile::ReadOnly))
    {
        return tr("Unable to open dictionary file ") + dictFile.fileName() + "\n\n" + dictFile.errorString();
    }

    // Start searching from the middle
    qint64 left = 0;
    qint64 right = dictFile.size() - 4096;
    dictFile.seek((left + right) / 2);

    // 0 = find some matching expression
    // 1 = go forward for first matching expr
    // 2 = appending text inside matching entry
    // 3 = skipping text outside entry
    int phase = 0;

    QString exprString("<entry key=\"" + expr);
    char buf[4096];    
    QString result;
    int numResults = 0;
    for(;;)
    {
        int readRes = dictFile.readLine(&buf[0], 4096);
        if(readRes < 0)
        {
            if(dictFile.atEnd())
            {
                break;
            }
            else
            {
                result += tr("Error reading from dictionary file") + ":\n\n" + dictFile.errorString();
            }
            break;
        }
        if(readRes == 0)
        {
            continue;   // empty line
        }
        if(phase == 2)
        {
            QString line(buf);
            int entryEnd = line.indexOf("</entry>");
            if(entryEnd < 0)
            {
                result += line;
                continue;
            }
            result += line.left(entryEnd + 8);
            numResults++;
            if(numResults > maxResults)
            {
                break;
            }
            phase = 3;
            continue;
        }
        char *keyStart = strstr(buf, "<entry key=\"");
        if(keyStart == 0)
        {
            continue;
        }
        keyStart += 12;
        char *keyEnd = strchr(keyStart, '"');
        QString key = QString::fromUtf8(keyStart, keyEnd - keyStart);
        int cmp = compareExprKey(expr, key);
        if(cmp == UNCOMPARABLE_CHARS)
        {
            continue;        // skip uncomparable words
        }
        if(phase == 0)
        {
            bool changed = true;
            if(cmp > 0)      // expression is bigger then key
            {
                left = dictFile.pos();
            }
            else            // expression is smaller or matches
            {
                changed = (right != dictFile.pos());    // comparing twice same word
                right = dictFile.pos();
            }
            if(changed && (right - left > 4096))
            {
                dictFile.seek((left + right) / 2);
                continue;
            }
            phase = 1;
            dictFile.seek(left);
            continue;
        }
        if(phase == 1)
        {
            if(cmp > 0)
            {
                continue;           // first match still not found
            }
            else if(cmp < 0)
            {
                break;              // all matching words passed
            }
            phase = 2;
        }
        if(phase == 2 || phase == 3)
        {
            QString str = QString::fromUtf8(buf);
            int entryStart = str.indexOf(exprString, 0, Qt::CaseInsensitive);
            if(entryStart < 0)
            {
                break;      // first non matching entry was hit
            }
            result += str.right(entryStart - exprString.length());
            phase = 2;
        }
    }
    dictFile.close();
    if(result.length() == 0)
    {
        result = tr("Expression not found");
    }
    return result;
}

static QString toHtml(QString &xml)
{
    QByteArray bytes = xml.toUtf8();
    QBuffer buf(&bytes);
    QXmlInputSource source(&buf);
    GcideXmlHandler handler;
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(source);
    return handler.html;
}

void Dict::textChanged(const QString &text, QObject *browser)
{
    //qDebug() << "text changed" << text;
    QString xml = "<entries>";
    xml += searchExpr(text, 32);
    xml += "</entries>";
    QString html = toHtml(xml);
    browser->setProperty("text", html);
    //qDebug() << "html=" << html;
    //browser->setText(html);
}

GcideXmlHandler::GcideXmlHandler()
{
}

bool GcideXmlHandler::startElement(const QString & /* namespaceURI */,
                                   const QString & /* localName */,
                                   const QString &qName,
                                   const QXmlAttributes &attributes)
{
    if(qName == "ex")               // expression
    {
        html += "<b>";
    }
    else if(qName == "def")         // definition
    {
        skip = false;
    }
    else if(qName == "sn")          // senses <sn no="1">....</sn>
    {
        if(html.endsWith("</ol>"))
        {
            html.chop(5);
        }
        else
        {
            html += "<ol>";
        }
        html += "<li>";
        skip = false;
    }
    if(qName == "entry")            // <entry key="Hell">
    {
        html += "<h1>";
        html += attributes.value("key");
        html += "</h1>";
        skip = true;
    }
    else if(qName == "entries")
    {
        html += "<html>";
    }

    return true;
}

bool GcideXmlHandler::endElement(const QString & /* namespaceURI */,
                                 const QString & /* localName */,
                                 const QString &qName)
{
    if(qName == "ex")
    {
        html += "</b>";
    }
    else if(qName == "def")
    {
        skip = true;
    }
    else if(qName == "sn")
    {
        html += "</li></ol>";
    }
    else if(qName == "entry")
    {
        html += "<br><br>";
    }
    else if(qName == "entries")
    {
        html += "</html>";
    }
    return true;
}

bool GcideXmlHandler::characters(const QString &str)
{
    if(skip)
    {
        return true;
    }
    html += str.trimmed();
    return true;
}


bool GcideXmlHandler::fatalError(const QXmlParseException &exception)
{
    html += QObject::tr("Parse error at line %1, column %2:\n%3")
            .arg(exception.lineNumber())
            .arg(exception.columnNumber())
            .arg(exception.message());
    return false;
}

