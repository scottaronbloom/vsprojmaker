// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __DEBUGCMD_H
#define __DEBUGCMD_H

#include <QString>
#include <QMap>
#include <QDataStream>

namespace NVSProjectMaker
{
    struct SDebugCmd
    {
        QString fSourceDir;
        QString fName;
        QString fCmd;
        QString fArgs;
        QString fWorkDir;
        QString fEnvVars;

        QString getEnvVars() const;
        QString getProjectName() const;
        QString getCmd() const;
        void cleanUp( const QMap< QString, QString > & map );
    };
}

QDataStream & operator<<( QDataStream & stream, const NVSProjectMaker::SDebugCmd & value );
QDataStream & operator>>( QDataStream & stream, NVSProjectMaker::SDebugCmd & value );
QDataStream & operator<<( QDataStream & stream, const QList< NVSProjectMaker::SDebugCmd > & value );
QDataStream & operator>>( QDataStream & stream, QList< NVSProjectMaker::SDebugCmd > & value );

Q_DECLARE_METATYPE( NVSProjectMaker::SDebugCmd );

#endif 

