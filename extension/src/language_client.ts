// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

import * as path from "path";
import * as vscode from "vscode";
import { ExtensionContext, OutputChannel, window, workspace } from "vscode";
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind,
} from "vscode-languageclient/node";

let client: LanguageClient | undefined;
let ttx_channel: OutputChannel | undefined;

const semantic_highlighting_setting = "semanticHighlighting.enabled";
const packages_root_setting = "packagesRoot";
const package_sources_setting = "packageSources";

interface PackageSource {
  argument: string;
  root: string;
}

function package_sources(language_id: string): PackageSource[] {
  const configured = workspace
    .getConfiguration(language_id)
    .get<Record<string, string>>(package_sources_setting, {});
  return Object.entries(configured).map(([coordinate, configured_root]) => {
    const separator = coordinate.lastIndexOf("@");
    const identity = coordinate.slice(0, separator);
    const version = coordinate.slice(separator + 1);
    const root = resolve_configured_path(configured_root);
    return {
      argument: `-package-source=${identity}|${version}|${root}`,
      root,
    };
  });
}

function resolve_configured_path(configured: string): string {
  const workspace_root = workspace.workspaceFolders?.[0]?.uri.fsPath;
  return workspace_root && !path.isAbsolute(configured)
    ? path.resolve(workspace_root, configured)
    : configured;
}

// TextMate remains the default color owner. The semantic middleware suppresses
// token requests until the setting explicitly opts the document into them.
function semantic_highlighting_enabled(
  language_id: string,
  document?: vscode.TextDocument
): boolean {
  return workspace
    .getConfiguration(language_id, document?.uri)
    .get<boolean>(semantic_highlighting_setting, false);
}

function follows_call_operator(
  document: vscode.TextDocument,
  change: vscode.TextDocumentContentChangeEvent
): boolean {
  if (change.text !== " " || change.range.start.line >= document.lineCount) {
    return false;
  }

  const line = document.lineAt(change.range.start.line).text;
  const end = change.range.start.character + change.text.length;
  return end <= line.length && line.slice(0, end).endsWith("-> ");
}

export function start_language_client(
  context: ExtensionContext,
  language_id: string
): void {
  // One client owns the packaged Puffer process, diagnostics, hover, tokens,
  // and formatting for the extension lifetime.
  const language_selector = { language: language_id, scheme: "file" };
  ttx_channel = window.createOutputChannel("TTX Language Server");
  context.subscriptions.push(ttx_channel);

  const server_path = context.asAbsolutePath(path.join(".", "puffer"));
  const configured_packages_root = workspace
    .getConfiguration(language_id)
    .get<string>(packages_root_setting, "");
  const packages_root = configured_packages_root
    ? resolve_configured_path(configured_packages_root)
    : context.asAbsolutePath("packages");
  const configured_package_sources = package_sources(language_id);
  const repository_arguments = [
    `-packages-root=${packages_root}`,
    ...configured_package_sources.map((source) => source.argument),
  ];
  ttx_channel.appendLine(`Launching Puffer LSP using path: ${server_path}`);

  const server_options: ServerOptions = {
    run: {
      command: server_path,
      args: repository_arguments,
      transport: TransportKind.pipe,
    },
    debug: {
      command: server_path,
      args: repository_arguments,
      transport: TransportKind.pipe,
    },
  };

  const client_options: LanguageClientOptions = {
    documentSelector: [language_selector],
    outputChannel: ttx_channel,
    middleware: {
      handleDiagnostics: (uri, diagnostics, next) => {
        ttx_channel?.appendLine(`Received diagnostics for ${uri}:`);
        diagnostics.forEach((diagnostic) =>
          ttx_channel?.appendLine(`  ${diagnostic.message}`)
        );
        return next(uri, diagnostics);
      },
      provideDocumentSemanticTokens: (document, token, next) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, token);
      },
      provideDocumentSemanticTokensEdits: (
        document,
        previous_result_id,
        token,
        next
      ) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, previous_result_id, token);
      },
      provideDocumentRangeSemanticTokens: (document, range, token, next) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, range, token);
      },
    },
    synchronize: {
      // Package dependencies include source and arbitrary embedded resources.
      // Puffer confines each event to active Package roots before invalidating
      // the complete Workspace transaction.
      fileEvents: [
        workspace.createFileSystemWatcher("**/*"),
        ...(configured_packages_root
          ? [
              workspace.createFileSystemWatcher(
                new vscode.RelativePattern(packages_root, "**/*")
              ),
            ]
          : []),
        ...configured_package_sources.map((source) =>
          workspace.createFileSystemWatcher(
            new vscode.RelativePattern(source.root, "**/*")
          )
        ),
      ],
    },
  };

  const language_client = new LanguageClient(
    "TetrodotoxinLanguageServer",
    "TTX Language Server",
    server_options,
    client_options
  );
  client = language_client;

  language_client.onDidChangeState((event) => {
    ttx_channel?.appendLine(
      `Client state changed: ${event.oldState} -> ${event.newState}`
    );
  });

  language_client.start();
  ttx_channel.appendLine("LSP client started.");
  ttx_channel.appendLine(
    `Semantic highlighting: ${
      semantic_highlighting_enabled(language_id) ? "enabled" : "disabled"
    }`
  );

  context.subscriptions.push(
    workspace.onDidChangeConfiguration((event) => {
      if (
        event.affectsConfiguration(
          `${language_id}.${semantic_highlighting_setting}`
        )
      ) {
        ttx_channel?.appendLine(
          `Semantic highlighting: ${
            semantic_highlighting_enabled(language_id)
              ? "enabled"
              : "disabled"
          }`
        );
        void vscode.commands
          .executeCommand("editor.action.restartSemanticTokens")
          .then(undefined, () => undefined);
      }
    })
  );

  context.subscriptions.push(
    workspace.onDidChangeTextDocument((event) => {
      const editor = window.activeTextEditor;
      const change = event.contentChanges[event.contentChanges.length - 1];
      if (
        !editor ||
        editor.document !== event.document ||
        event.document.languageId !== language_id ||
        !change ||
        !follows_call_operator(event.document, change)
      ) {
        return;
      }

      // Typing the preferred trailing space closes the suggestions opened by
      // `>`. Reopening them after document synchronization keeps ` -> ` useful
      // without making every ordinary space a completion trigger.
      setTimeout(() => {
        void vscode.commands.executeCommand("editor.action.triggerSuggest");
      }, 0);
    })
  );

}

export function deactivate_language_client(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  ttx_channel?.appendLine("LSP client stopped.");
  return client.stop();
}
