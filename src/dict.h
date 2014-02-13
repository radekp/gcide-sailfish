#ifndef QGCIDE_H
#define QGCIDE_H

#include <QDir>
#include <QBuffer>
#include <QXmlSimpleReader>

#define UNCOMPARABLE_CHARS 0x12345678

class Dict : public QObject
{
    Q_OBJECT
public:
    Dict(QObject* parent = 0);

private:
    QFile dictFile;
    QString searchExpr(const QString &, int maxResults);

public slots:
    void textChanged(const QString &, QObject *);

signals:
    void progressChanged(const QString &, int, int);

};

class GcideXmlHandler : public QXmlDefaultHandler
{
public:
    GcideXmlHandler();

    bool startElement(const QString &namespaceURI, const QString &localName,
                      const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                    const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);

    bool skip;
    QString html;
};

#endif
