module.exports = [
  {
    type: 'heading',
    defaultValue: 'WasteWatch'
  },
  {
    type: 'text',
    defaultValue: 'Configure your waste collection schedule.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Connection'
      },
      {
        type: 'input',
        messageKey: 'Token',
        label: 'API Token',
        attributes: {
          placeholder: 'kiedywywoz.pl token',
          type: 'text'
        }
      },
      {
        type: 'input',
        messageKey: 'ProxyUrl',
        label: 'OCR Proxy URL (optional)',
        description: 'Leave empty to use default service',
        attributes: {
          placeholder: 'Default: built-in service',
          type: 'url'
        }
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Address'
      },
      {
        type: 'input',
        messageKey: 'StreetName',
        label: 'Street',
        attributes: {
          placeholder: 'e.g. 3 Maja',
          type: 'text'
        }
      },
      {
        type: 'input',
        messageKey: 'NumberName',
        label: 'House Number',
        attributes: {
          placeholder: 'e.g. 1',
          type: 'text'
        }
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save Settings'
  }
];
